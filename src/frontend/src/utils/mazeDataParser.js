/* 
    Converte o texto digitado em um vetor 
    de posições do labirinto.
*/
export function parsePath(text) {
  const matches =
    text.match(/\((\d+),\s*(\d+)\)/g);

  if (!matches) {
    return [];
  }

  return matches.map(item => {
    const numbers = item.match(/\d+/g);

    return {
      row: Number(numbers[0]),
      col: Number(numbers[1]),
    };
  });
}