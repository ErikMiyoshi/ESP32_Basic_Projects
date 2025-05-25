Create a venv

python -m venv venv

Installation

pip install fastapi uvicorn influxdb-client
pip install fastapi[all]


Run

activate venv
uvicorn main:app --reload