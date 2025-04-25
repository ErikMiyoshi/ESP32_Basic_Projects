from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS
from typing import Optional
import os

from datetime import datetime, timedelta, timezone
from collections import defaultdict


app = FastAPI()


app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

INFLUX_URL = "http://localhost:8086"
INFLUX_TOKEN = "mytoken123"
INFLUX_ORG = "my_org"
INFLUX_BUCKET = "my_bucket"
INFLUX_MEASUEMENT = "mqtt_consumer"
INFLUX_FIELDS = ["adc_0_value", "adc_1_value"]

client = InfluxDBClient(
    url=INFLUX_URL,
    token=INFLUX_TOKEN,
    org=INFLUX_ORG
)

query_api = client.query_api()

@app.get("/sensor")
def sensor(range: str = "-2h"):
    if range.endswith("m"):
        minutos = int(range[:-1])
        start_time = datetime.now(timezone(-timedelta(hours=3))) + timedelta(minutes=minutos)
    else:   
        #start_time = datetime(2025,4,22,7,44)
        start_time = datetime.now(timezone(-timedelta(hours=3))) - timedelta(seconds=30)

    start_time = start_time.isoformat()
    query = f'''
        from(bucket: "{INFLUX_BUCKET}")
        |> range(start: {start_time})
        |> filter(fn: (r) => r._measurement == "{INFLUX_MEASUEMENT}")
        |> filter(fn: (r) => r._field == "adc_0_value" or r._field == "adc_1_value")
        |> sort(columns: ["_time"], desc: true)
    '''
    query_api = client.query_api()
    tables = query_api.query(query)

    results = []
    for table in tables:
        for record in table.records:
            results.append({
                record.get_field(): record.get_value(),
                "timestamp": record.get_time().isoformat()
            })
    return results


@app.get("/sensor/last")
def sensor_last():
    query = f'''
        from(bucket: "{INFLUX_BUCKET}")
        |> range(start: -1d, stop: now())
        |> filter(fn: (r) => r["_measurement"] == "{INFLUX_MEASUEMENT}")
        |> filter(fn: (r) => r["_field"] == "adc_0_value" or r["_field"] == "adc_1_value")
        |> last()
    '''
    query_api = client.query_api()
    tables = query_api.query(query)
    results = []
    for table in tables:
        for record in table.records:          
            results.append({
                record.get_field(): record.get_value(),
                "timestamp": record.get_time().isoformat()
            })

    return results

@app.get("/sensor/interval")
def sensor_interval(start: str = "-6h", window: str = "1m"):
    query = f'''
        from(bucket: "{INFLUX_BUCKET}")
        |> range(start: {start})
        |> filter(fn: (r) => r._measurement == "{INFLUX_MEASUEMENT}")
        |> filter(fn: (r) => r._field == "adc_0_value" or r._field == "adc_1_value")          
        |> aggregateWindow(every: {window}, fn: mean)
    '''

    query_api = client.query_api()
    tables = query_api.query(query)
    results = []

    group = defaultdict(dict)
    for table in tables:
        for record in table.records:
            timestamp = record.get_time().isoformat()
            field = record.get_field()
            value = record.get_value()

            group[timestamp]["timestamp"] = timestamp
            group[timestamp][field] = value

    results = [group[ts] for ts in sorted(group)]
    return results