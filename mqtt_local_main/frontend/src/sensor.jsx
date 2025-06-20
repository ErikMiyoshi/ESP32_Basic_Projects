import React, { useEffect, useState } from "react";
import axios from "axios";

const Sensor = () => {
  const [sensor, setSensor] = useState(null);
  const [error, setError] = useState(null);

  useEffect(() => {
    const fetchSensor = () => {
      axios
        .get("http://127.0.0.1:8000/sensor/last")
        .then((response) => {
          console.log(response.data)
          setSensor(response.data);
        })
        .catch((err) => {
          console.error(err);
          setError("Error");
        });
    };

    // chama imediatamente
    fetchSensor();

    // atualiza a cada 2 segundos
    const interval = setInterval(fetchSensor, 100);

    // limpa o intervalo quando o componente desmonta
    return () => clearInterval(interval);
  }, []);

  if (error) return <p>{error}</p>;
  if (!sensor) return <p>Loading...</p>;

  return (
    <div>
      <h2>Sensor Now</h2>
      <p>ADC 0: {sensor[0].adc_0_value}</p>
      <p>ADC 1: {sensor[1].adc_1_value}</p>
    </div>
  );
};

export default Sensor;