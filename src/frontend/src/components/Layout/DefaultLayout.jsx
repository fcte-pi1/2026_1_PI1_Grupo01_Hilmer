import React from 'react';
import { Outlet } from 'react-router-dom';

function DefaultLayout({ backendStatus, backendMessage }) {
  return (
    <div>
      <header style={{ backgroundColor: 'var(--color-primary)', color: 'var(--color-text-light)', padding: '10px 20px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <h1>Micromouse PI1</h1>
        <div>
          Status do Backend: <span className={`status-badge ${backendStatus}`}>{backendStatus}</span>
          {backendMessage && <span style={{ marginLeft: '10px', fontSize: '0.9em' }}>({backendMessage})</span>}
        </div>
      </header>
      <main style={{ padding: '20px', flexGrow: 1 }}>
        <Outlet /> {/* Renderiza o conteúdo da rota filha aqui */}
      </main>
      <footer style={{ backgroundColor: 'var(--color-primary)', color: 'var(--color-text-light)', padding: '10px 20px', textAlign: 'center' }}>
        <p>&copy; 2026 Grupo PI1 - Micromouse</p>
      </footer>
    </div>
  );
}

export default DefaultLayout;