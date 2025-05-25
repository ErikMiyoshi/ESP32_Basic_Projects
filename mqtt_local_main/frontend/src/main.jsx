import React from 'react';
import ReactDOM from 'react-dom/client';
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import App from './App';
import SensorChart from './sensorchart';
import HistoryChart from './historychart';
import JoystickChart from './joystickchart';

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<SensorChart />} />
        <Route path="/info" element={<App />} />
        <Route path="/history" element={<HistoryChart />} />
        <Route path="/joystick" element={<JoystickChart />} />
      </Routes>
    </BrowserRouter>
  </React.StrictMode>
);