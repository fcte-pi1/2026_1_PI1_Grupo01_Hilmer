-- =============================================================
-- SCHEMA - Micromouse PI1
-- Grupo 01 - Hilmer 2026/1
-- =============================================================

CREATE TABLE IF NOT EXISTS HISTORICO (
    numTentativa      SERIAL      NOT NULL,
    percentualBateria FLOAT       NOT NULL,
    velocidadeMedia   FLOAT       NOT NULL,
    tempoConclusao    TIMESTAMP   NOT NULL,
    desafioCumprido   VARCHAR(3)  NOT NULL,
    correnteEletrica  FLOAT       NOT NULL,
    tensaoEletrica    FLOAT       NOT NULL,
    tipoLabirinto     VARCHAR(30) NOT NULL,

    CONSTRAINT pk_historico PRIMARY KEY (numTentativa),
    CONSTRAINT chk_desafio_cumprido CHECK (desafioCumprido IN ('SIM', 'NAO'))
);

CREATE TABLE IF NOT EXISTS TELEMETRIA (
    numTentativa    INT         NOT NULL,
    tempoColeta     TIMESTAMP   NOT NULL,
    tensaoRecente   FLOAT       NOT NULL,
    correnteRecente FLOAT       NOT NULL,
    posHRecente     INT         NOT NULL,
    posVRecente     INT         NOT NULL,
    velocidadeAtual FLOAT       NOT NULL,
    bateriaAtual    FLOAT       NOT NULL,
    tensaoAtual     FLOAT       NOT NULL,
    sensorCor       VARCHAR(25),
    sensorEsquerda  FLOAT,
    sensorDireita   FLOAT,
    sensorFrontal   FLOAT,

    CONSTRAINT pk_telemetria PRIMARY KEY (numTentativa, tempoColeta),
    CONSTRAINT fk_telemetria_historico FOREIGN KEY (numTentativa) REFERENCES HISTORICO(numTentativa) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS TRAJETO (
    numTentativa INT         NOT NULL,
    passo        INT         NOT NULL,
    pos_h        INT         NOT NULL,
    pos_v        INT         NOT NULL,
    direcao      VARCHAR(10) NOT NULL,

    CONSTRAINT pk_trajeto        PRIMARY KEY (numTentativa, passo),
    CONSTRAINT fk_trajeto_historico FOREIGN KEY (numTentativa) REFERENCES HISTORICO(numTentativa) ON DELETE CASCADE,
    CONSTRAINT chk_trajeto_direcao  CHECK (direcao IN ('NORTE', 'SUL', 'LESTE', 'OESTE'))
);