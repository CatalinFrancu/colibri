<?php

require_once 'smarty/Smarty.class.php';
define('BACKEND_PORT', 2359);

$fen = isset($_GET['fen']) ? $_GET['fen'] : "8/8/8/8/8/6R1/8/1k6 w - - 0 0";

list($board, $stm) = fenToBoard($fen);
list($score, $moves) = serverQuery($fen);

$smarty = new Smarty();
$smarty->template_dir = 'templates';
$smarty->compile_dir = 'templates_c';
$smarty->assign('board', $board);
$smarty->assign('stm', $stm);
$smarty->assign('score', $score);
$smarty->assign('moves', $moves);
$smarty->display('index.tpl');

/***************************************************************************/

/* Also sets the appropriate color on each square */
function fenToBoard($fen) {
  $board = array();
  list($pos, $stm, $ignored1, $epSquare, $ignored2, $ignored3) = explode(' ', $fen);
  $rows = explode('/', $pos);
  $rankNumber = 8;

  foreach ($rows as $row) {
    $fileName = 'a';
    $v = array();
    for ($i = 0; $i < strlen($row); $i++) {
      if (ctype_digit($row[$i])) {
        for ($j = 0; $j < $row[$i]; $j++) {
          $v[$fileName] = array('piece' => '');
          $fileName++;
        }
      } else {
        $side = ctype_upper($row[$i]) ? 'w' : 'b';
        $piece = strtolower($row[$i]);
        $v[$fileName] = array('piece' => $side . $piece);
        $fileName++;
      }
    }
    $board[$rankNumber--] = $v;
  }

  foreach ($board as $i => $row) {
    foreach ($row as $j => $square) {
      $board[$i][$j]['color'] = (($i + ord($j)) % 2) ? 'whiteBg' : 'blackBg';
    }
  }
  return array($board, $stm);
}

function serverQuery($fen) {
  $sock = @fsockopen("localhost", BACKEND_PORT);
  if (!$sock) {
    die("Backend server not responding");
  }
  fprintf($sock, "egtb $fen\n");
  list($score, $numMoves) = fscanf($sock, "%d %d");
  $children = array();
  for ($i = 0; $i < $numMoves; $i++) {
    $line = fgets($sock);
    list($move, $childScore, $childFen) = explode(' ', $line, 3);
    $children[] = array('move' => $move, 'score' => -$childScore, 'fen' => $childFen);
  }
  fclose($sock);
  usort($children, "cmpChildren");
  return array($score, $children);
}

function cmpChildren($a, $b) {
  $sa = sign($a['score']);
  $sb = sign($b['score']);
  if ($sa != $sb) {
    return ($sa < $sb) ? 1 : -1;
  }
  if ($a['score'] != $b['score']) {
    return ($a['score'] < $b['score']) ? -1 : 1;
  }
  return ($a['move'] < $b['move']) ? -1 : 1;
}

/* Returns -1 / 0 / 1 for an integer argument */
function sign($int) {
  return min(1, max(-1, $int));
}

?>
