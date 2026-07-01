export function formatTime(seconds) {
  const m = Math.floor(seconds / 60).toString().padStart(2, '0');
  const s = (seconds % 60).toString().padStart(2, '0');
  return `${m}:${s}`;
}

export function formatSpeed(metersPerSecond) {
  return `${metersPerSecond.toFixed(2)} m/s`;
}

export function formatBattery(percent) {
  return `${percent}%`;
}

export function formatMazeDimension(size) {
  return `${size}x${size}`;
}

export function statusLabel(status) {
  const labels = {
    running: 'EM ANDAMENTO',
    success: 'SUCESSO',
    failure: 'FALHA',
    stuck: 'TRAVADO',
    waiting: 'AGUARDANDO INÍCIO',
    idle: 'AGUARDANDO',
  };
  return labels[status] ?? status;
}

export function filterExecutions(executions, filters) {
  
  return executions.filter((exec) => {
    if (filters.mazeSize.length > 0 && !filters.mazeSize.includes(`${exec.mazeSize}x${exec.mazeSize}`))
      return false;
    
    const executionStatus = exec.status === "success" ? "Sucesso" : "Falha";
    
    if (filters.status.length > 0 && !filters.status.includes(executionStatus))       
      return false;

    if (filters.totalTime.length > 0) {
      const matchesTime = filters.totalTime.some((range) => {
        const t = exec.totalTimeSeconds;
        switch (range) {
          case "[1s - 59 s]":
            return t >= 1 && t <= 59;
          case "[1m - 2m]":
            return t >= 60 && t <= 120;
          case "maior do que 2m":
            return t > 120;
          default:
            return false;
        }
      });

      if (!matchesTime) 
        return false;
    }

    if (filters.avgSpeed.length > 0) {
      const matchesSpeed = filters.avgSpeed.some((range) => {
        const s = exec.avgSpeedMps;
        switch (range) {
          case "[0.01 m/s - 0.59 m/s]":
            return s >= 0.01 && s <= 0.59;
          case "[1.0 m/s - 1.59 m/s]":
            return s >= 1.0 && s <= 1.59;
          case "maior do que 2 m/s":
            return s > 2;
          default:
            return false;
        }
      });

      if (!matchesSpeed) 
        return false;
      
    }

    if (filters.totalBatteryUsed.length > 0) {
      const matchesConsumption = filters.totalBatteryUsed.some((value) => {
        switch (value) {
          case "menor do que 50%":
            return exec.totalBatteryUsed < 50;
          case "maior ou igual a 50%":
            return exec.totalBatteryUsed >= 50;
          default:
            return false;
          }
        });

      if (!matchesConsumption) 
        return false;
      
    }
    return true;
  });
}