import { useEffect, useState } from "react";

const apiBaseUrl = import.meta.env.VITE_API_BASE_URL || "/api";

function App() {
  const [backendStatus, setBackendStatus] = useState("carregando");
  const [backendMessage, setBackendMessage] = useState("");

  const statusClassName =
    backendStatus === "online"
      ? "status-badge status-online"
      : backendStatus === "offline"
        ? "status-badge status-offline"
        : "status-badge status-loading";

  useEffect(() => {
    let cancelled = false;

    async function loadStatus() {
      try {
        const response = await fetch(`${apiBaseUrl}/health`);

        if (!response.ok) {
          throw new Error(`Falha ao consultar backend (${response.status})`);
        }

        const payload = await response.json();

        if (!cancelled) {
          setBackendStatus(payload.status || "online");
          setBackendMessage(payload.message || "Backend respondeu.");
        }
      } catch (error) {
        if (!cancelled) {
          setBackendStatus("offline");
          setBackendMessage(error.message);
        }
      }
    }

    loadStatus();

    return () => {
      cancelled = true;
    };
  }, []);

  return (
    <main className="page">
      <header className="hero">
        <p className="eyebrow">PI1</p>
        <h1>Projeto Integrador 1</h1>
        <p className="subtitle">
          Base inicial do sistema com frontend em React + Vite e backend em
          Node.js.
        </p>
      </header>

      <section className="card">
        <div className="section-header">
          <h2>Status do backend</h2>
          <span className={statusClassName}>{backendStatus}</span>
        </div>
        <p>{backendMessage}</p>
      </section>

      <section className="card">
        <h2>Pendências</h2>
        <ul>
          <li>Definir banco de dados</li>
          <li>Definir hardware dos sensores</li>
          <li>Definir fluxo HTTP entre sensores e backend</li>
        </ul>
      </section>
    </main>
  );
}

export default App;
