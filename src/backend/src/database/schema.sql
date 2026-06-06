-- =============================================================
-- SCHEMA - Micromouse PI1
-- Grupo 01 - Hilmer 2026/1
-- =============================================================

CREATE TABLE IF NOT EXISTS HISTORICO (
    numTentativa      INT          NOT NULL,
    percentualBateria FLOAT        NOT NULL,
    velocidadeMedia   FLOAT        NOT NULL,
    tempoConclusao    TIMESTAMP    NOT NULL,
    desafioCumprido   VARCHAR(3)      NOT NULL,
    correnteEletrica  FLOAT        NOT NULL,
    tensaoEletrica    FLOAT        NOT NULL,
    tipoLabirinto     VARCHAR(5)   NOT NULL,

    CONSTRAINT pk_historico           PRIMARY KEY (numTentativa),
    CONSTRAINT chk_desafio_cumprido   CHECK (desafioCumprido IN ('SIM', 'NAO')),
    CONSTRAINT chk_percentual_bateria CHECK (percentualBateria BETWEEN 0.0 AND 100.0),
    CONSTRAINT chk_tipo_labirinto     CHECK (tipoLabirinto IN ('4x4', '8x8', '16x16'))
);

CREATE TABLE IF NOT EXISTS TELEMETRIA (
    numTentativa    INT          NOT NULL,
    tempoColeta     TIMESTAMP    NOT NULL,
    tensaoRecente   FLOAT        NOT NULL,
    correnteRecente FLOAT        NOT NULL,
    posHRecente     INT          NOT NULL,
    posVRecente     INT          NOT NULL,
    velocidadeAtual FLOAT        NOT NULL,
    bateriaAtual    FLOAT        NOT NULL,
    tensaoAtual     FLOAT        NOT NULL,
    sensorCor       CHAR(7),
    sensorEsquerda  FLOAT,
    sensorDireita   FLOAT,
    sensorFrontal   FLOAT,

    CONSTRAINT pk_telemetria           PRIMARY KEY (numTentativa, tempoColeta),
    CONSTRAINT fk_telemetria_historico FOREIGN KEY (numTentativa) REFERENCES HISTORICO(numTentativa) ON DELETE CASCADE,
    CONSTRAINT chk_bateria_atual       CHECK (bateriaAtual BETWEEN 0.0 AND 100.0),
    CONSTRAINT chk_sensor_cor          CHECK (sensorCor ~ '^#[0-9A-Fa-f]{6}$')
);

CREATE TABLE IF NOT EXISTS TRAJETO (
    numTentativa INT        NOT NULL,
    passo        INT        NOT NULL,
    pos_h        INT        NOT NULL,
    pos_v        INT        NOT NULL,   
    direcao      VARCHAR(6) NOT NULL,

    CONSTRAINT pk_trajeto           PRIMARY KEY (numTentativa, passo),
    CONSTRAINT fk_trajeto_historico FOREIGN KEY (numTentativa) REFERENCES HISTORICO(numTentativa) ON DELETE CASCADE,
    CONSTRAINT chk_trajeto_direcao  CHECK (direcao IN ('NORTE', 'SUL', 'LESTE', 'OESTE')),
    CONSTRAINT chk_trajeto_passo    CHECK (passo >= 1)
);