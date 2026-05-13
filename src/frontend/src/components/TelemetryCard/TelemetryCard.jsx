import React from 'react';
import './TelemetryCard.module.css';

function TelemetryCard({ title, value }) {
  return (
    <div className="telemetry-card">
      <h3>{title}</h3>
      <p>{value}</p>
    </div>
  );
}

export default TelemetryCard;