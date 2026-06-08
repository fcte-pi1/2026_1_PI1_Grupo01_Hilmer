import { WebSocketServer } from 'ws';

const PORTA = 8080;
const wss = new WebSocketServer({ port: PORTA });
const tempoInicial = Date.now(); 

console.log(`Simulador da ESP32-C3 rodando em ws://127.0.0.1:${PORTA}`);

wss.on('connection', (ws) => {
  console.log('Conexão recebida! (O Backend Node.js conectou no simulador)');

  const intervalo = setInterval(() => {
    
    const matrizTamanho = 33;
    let matrizLabirinto = [];
    
    for (let i = 0; i < matrizTamanho; i++) {
      let linha = [];
      for (let j = 0; j < matrizTamanho; j++) {
        
        if (i === 0 || i === matrizTamanho - 1 || j === 0 || j === matrizTamanho - 1) {
          linha.push(1);
        } else {
          
          const rand = Math.random();
          if (rand < 0.2) linha.push(1);      
          else if (rand < 0.6) linha.push(0); 
          else linha.push(2);                 
        }
      }
      matrizLabirinto.push(linha);
    }

    const tempoDecorridoms = Date.now() - tempoInicial; 

    
    const novaTelemetria = {
      tipoLabirinto: "16x16", 
      bateriaConsumo: (Math.random() * 100).toFixed(1), 
      velocidadeMedia: (0.2 + Math.random() * 0.6).toFixed(2), 
      tempoConclusao: (tempoDecorridoms / 1000).toFixed(1), 
      desafioCumprido: Math.random() > 0.8 ? "S" : "N", 
      mapa: matrizLabirinto
    };
    
    ws.send(JSON.stringify(novaTelemetria));
  }, 100);

  ws.on('close', () => {
    console.log('Backend desconectou do simulador.');
    clearInterval(intervalo);
  });
});