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

const JoystickChart = () => {
  const [dados, setDados] = useState([]);

  useEffect(() => {
    const buscarDados = () => {
      axios.get("http://127.0.0.1:8000/sensor/last")
        .then((res) => {
          const novo = {
            timestamp: res.data[0].timestamp,
            y: res.data[0].adc_0_value,
            x: res.data[1].adc_1_value,
          };
          
          // Mantém os últimos 20 valores
          setDados((prev) => [...prev.slice(-50), novo]);
        })
        .catch((err) => console.error("Erro ao buscar dados:", err));
    };
    console.log(dados)
    buscarDados();
    const interval = setInterval(buscarDados, 30);
    return () => clearInterval(interval);
  }, []);

  return (
    <div>
        <nav><Link to="/">Home</Link></nav>

      <h2>Joystick position</h2>
      <LineChart width={600} height={400} data={dados}>
        <CartesianGrid stroke="#ccc" />
        <XAxis type="number" dataKey="x" domain={[0, 6400]} />
        <YAxis type="number" dataKey="y" domain={[0, 6400]} />
        <Tooltip />
        <Line
            data={dados}
            type="linear"
            dataKey="y"
            stroke="#8884d8"
            dot={{ r: 2 }}
            isAnimationActive={false}
        />
        </LineChart>
    </div>
  );
};

export default JoystickChart;
