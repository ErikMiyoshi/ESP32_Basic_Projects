import { useEffect, useState } from 'react';
import axios from 'axios';
import { LineChart, Line, XAxis, YAxis, Tooltip, CartesianGrid } from 'recharts';
import { Link } from 'react-router-dom';
import { format } from 'date-fns';

function HistoryChart() {
  const [data, setData] = useState([]);
  const [interval, setInterval] = useState("-30m");
  const [window, setWindow] = useState("1m");

  useEffect(() => {
    axios.get(`http://127.0.0.1:8000/sensor/interval?start=${interval}&window=${window}`)
      .then(res => {


        setData(res.data);
      });
  }, [interval, window]);

  return (
    <div>
      <nav><Link to="/">Real Time Chart</Link></nav>

      <h2>Sensor history</h2>
      
      <label>
        Interval:
        <select value={interval} onChange={e => setInterval(e.target.value)}>
          <option value="-5m">Last 5 minutes</option>
          <option value="-30m">Last 30 minutes</option>
          <option value="-1h">Last 1 hora</option>
          <option value="-6h">Last 6 horas</option>
        </select>
      </label>
      <label>
        Window:
        <select value={window} onChange={e => setWindow(e.target.value)}>
          <option value="1m">1 minutes</option>
          <option value="2m">2 minutes</option>
          <option value="5m">5 minutes</option>
          <option value="10m">10 minutes</option>
        </select>
      </label>

      <LineChart width={800} height={300} data={data}>
        <XAxis dataKey="timestamp" tickFormatter={(value) => format(new Date(value), 'HH:mm')} />
        <YAxis />
        <CartesianGrid stroke="#ccc" />
        <Tooltip />
        <Line type="monotone" dataKey="adc_0_value" stroke="#8884d8" />
        <Line type="monotone" dataKey="adc_1_value" stroke="#82ca9d" />
      </LineChart>
    </div>
  );
}

export default HistoryChart;