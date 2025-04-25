// src/TemperaturaChart.jsx
import { Link } from 'react-router-dom';
import React, { useEffect, useState } from "react";
import { format } from 'date-fns';
import axios from "axios";
import {
  LineChart,
  Line,
  CartesianGrid,
  XAxis,
  YAxis,
  Tooltip,
} from "recharts";

const SensorChart = () => {
  const [dados, setDados] = useState([]);

  useEffect(() => {
    const buscarDados = () => {
      axios.get("http://127.0.0.1:8000/sensor/last")
        .then((res) => {
          const novo = {
            timestamp: res.data[0].timestamp,
            adc_0: res.data[0].adc_0_value,
            adc_1: res.data[1].adc_1_value,
          };
          // Mantém os últimos 20 valores
          setDados((prev) => [...prev.slice(-99), novo]);
        })
        .catch((err) => console.error("Erro ao buscar dados:", err));
    };

    buscarDados();
    const interval = setInterval(buscarDados, 100);
    return () => clearInterval(interval);
  }, []);

  return (
    <div>
        <nav><Link to="/history">History Chart</Link></nav>

      <h2>Gráfico em Tempo Real</h2>
      
      <LineChart width={600} height={300} data={dados}>
        <Line type="monotone" dataKey="adc_0" stroke="#8884d8" />
        <Line type="monotone" dataKey="adc_1" stroke="#82ca9d" />
        <CartesianGrid stroke="#ccc" />
        <XAxis dataKey="timestamp" tickFormatter={(value) => format(new Date(value), 'mm:ss')} />
        <YAxis />
        <Tooltip />
      </LineChart>
    </div>
  );
};

export default SensorChart;
