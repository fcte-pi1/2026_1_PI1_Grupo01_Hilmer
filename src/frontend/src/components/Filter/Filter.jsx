import styles from "./Filter.module.css";

export function Filter({ isOpen, onClose, filters, setFilters }) {

  function handleFilterChange (category, value) {
    setFilters((prev) => ({
      ...prev, [category] : 
      prev[category].includes(value) ? 
      prev[category].filter((item) => item !== value) : [...prev[category], value],
    }));
  };

  return (
    <>
      {isOpen && (
        <div
          className={styles.backdrop}
          onClick={onClose}
        />
      )}

      <aside
        className={`${styles.sidebar} ${
          isOpen ? styles.open : ""
        }`}
      >
        <div className={styles.header}>
          <h2>Filtros</h2>
          <button onClick={onClose}>✕</button>
        </div>

        <ListOptions
          title="DIMENSÃO"
          category="mazeSize"
          listSet={["10x10", "12x12", "14x14", "16x16", "18x18", "20x20"]}
          filters={filters}
          onChange={handleFilterChange}
        />

        <ListOptions
          title="TEMPO"
          category="totalTime"
          listSet={["[1s - 59 s]", "[1m - 2m]", "maior do que 2m"]}
          filters={filters}
          onChange={handleFilterChange}
        />

        <ListOptions
          title="VEL. MÉDIA"
          category="avgSpeed"
          listSet={["[0.01 m/s - 0.59 m/s]", "[1.0 m/s - 1.59 m/s]", "maior do que 2 m/s"]}
          filters={filters}
          onChange={handleFilterChange}
        />
{ /*
        <ListOptions
          title="TENSÃO"
          listSet={["6,6V", "Maior que 6,6V"]}
          filters={filters}
          onChange={handleFilterChange}
        />

        <ListOptions
          title="CORRENTE"
          listSet={[ "[0.01 A - 0.59 A]", "[1.0 A - 1.59 A]", "maior do que 2 A"]}
          filters={filters}
          onChange={handleFilterChange}
        />
*/}
        <ListOptions
          title="CONSUMO"
          category="totalBatteryUsed"
          listSet={["menor do que 50%", "maior ou igual a 50%"]}
          filters={filters}
          onChange={handleFilterChange}
        />

        <ListOptions
          title="TENTATIVA CUMPRIDA"
          category="status"
          listSet={["Sucesso", "Falha"]}
          filters={filters}
          onChange={handleFilterChange}
        />
      </aside>
    </>
  );
}

function ListOptions({ title, listSet, category, filters, onChange }) {
  return (
    <section>
      <h3>{title}</h3>
      <ul>
        {listSet.map((item) => (
          <li key={item}>
            <label>
              <input 
                type="checkbox" 
                checked={filters[category].includes(item)} 
                onChange={() => onChange(category, item)} 
              />
                {item}
            </label>
          </li>
        ))}
      </ul>
    </section>
  );
}

