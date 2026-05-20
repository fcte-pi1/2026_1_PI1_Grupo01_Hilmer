import { canMove } from "./mazeHelpers";

// Executa a animação do robô no labirinto.
export function startMazeAnimation({ input, maze, rows, cols, 
                                     goal, parsePath, revealAround, 
                                    markVisited, setRobot }) 
    {

    const parsed = parsePath(input);
    
    // Não inicia se não houver trajeto.
    if (parsed.length === 0) {
        return;
    }

    let currentPosition = parsed[0];

    // Impede iniciar em uma parede.
    if (maze[currentPosition.row][currentPosition.col] === 1) {
        alert("Posição inicial não pode ser parede!");
        return;
    }

    let index = 1;

    // Define posição inicial do micromouse.
    setRobot(currentPosition);

    // Revela células ao redor da posição inicial.
    revealAround(currentPosition.row, currentPosition.col);

    // Primeira célula marcada como visitada.
    markVisited(currentPosition.row, currentPosition.col);

    // Executa movimentação em intervalos de 300 ms.
    const interval = setInterval(() => {

        // Finaliza ao chegar no fim do trajeto.
        if (index >= parsed.length) {
            clearInterval(interval);
            return;
        }

        const next = parsed[index];

        // Verifica se o movimento é válido.
        const validMove = canMove(currentPosition, next, maze, rows, cols);

        if (!validMove) {

            revealAround(currentPosition.row, currentPosition.col);
            clearInterval(interval);

            return;
        }

        // Atualiza posição do micromouse.
        setRobot(next);
        revealAround(next.row, next.col);
        markVisited(next.row, next.col);

        // Alcançou o objetivo? Finalize então
        if (next.row === goal.row && next.col === goal.col) {
            clearInterval(interval);
            return;
        }
        currentPosition = next;
        index++;

    }, 300);
}