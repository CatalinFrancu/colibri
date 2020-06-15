<?php

require_once 'smarty3/Smarty.class.php';
define('BACKEND_PORT', 2359);

$fen = isset($_GET['fen']) ? $_GET['fen'] : null;

if ($fen) {
  list($board, $stm) = fenToBoard($fen);
} else {
  list($board, $stm) = randomBoard(5);
  $fen = boardToFen($board, $stm);
}
colorBoard($board);
list($score, $scoreText, $children, $error) = serverQuery($fen);

$smarty = new Smarty();
$smarty->template_dir = 'templates';
$smarty->compile_dir = 'templates_c';
$smarty->assign('fen', $fen);
$smarty->assign('board', $board);
$smarty->assign('stm', $stm);
$smarty->assign('score', $score);
$smarty->assign('scoreText', $scoreText);
$smarty->assign('children', $children);
$smarty->assign('error', $error);
$smarty->assign('template', 'index.tpl');
$smarty->display('layout.tpl');

/***************************************************************************/

function randomBoard($numPieces) {
  // No queens -- they generate boring convert-in-2 positions.
  $pieceNames = [ 'p', 'n', 'b', 'r', 'k' ];
  $board = [];
  for ($i = 8; $i >= 1; $i--) {
    for ($j = 'a'; $j <= 'h'; $j++) {
      $board[$i][$j]['piece'] = '';
    }
  }
  for ($k = 0; $k < $numPieces; $k++) {
    do {
      $i = rand(1, 8);
      $j = chr(ord('a') + rand(0, 7));
    } while ($board[$i][$j]['piece']);
    $color = ($k < 2) ? $k : rand(0, 1); // First two pieces to be placed are of opposite colors
    $minPiece = ($i == 1 || $i == 8) ? 1 : 0; // No pawns on first/last rank
    $piece =  $pieceNames[rand($minPiece, count($pieceNames) - 1)];
    $board[$i][$j]['piece'] = ($color ? 'w' : 'b') . $piece;
  }
  $stm = (rand(0, 1) == 1) ? 'w' : 'b';
  return [ $board, $stm ];
}

function boardToFen($board, $stm) {
  $fen = '';
  for ($rank = 8; $rank >= 1; $rank--) {
    if ($rank < 8) {
      $fen .= '/';
    }
    $numEmpty = 0;
    for ($file = 'a'; $file <= 'h'; $file++) {
      $str = $board[$rank][$file]['piece'];
      if (!$str) {
        $numEmpty++;
      } else {
        if ($numEmpty) {
          $fen .= $numEmpty;
          $numEmpty = 0;
        }
        $fen .= ($str[0] == 'w') ? strtoupper($str[1]) : $str[1];
      }
    }
    if ($numEmpty) {
      $fen .= $numEmpty;
    }
  }
  $fen .= " $stm - - 0 0";
  return $fen;
}

function fenToBoard($fen) {
  $board = [];
  list($pos, $stm, $ignored1, $epSquare, $ignored2, $ignored3) = explode(' ', $fen);
  $rows = explode('/', $pos);
  $rankNumber = 8;

  foreach ($rows as $row) {
    $fileName = 'a';
    $v = [];
    for ($i = 0; $i < strlen($row); $i++) {
      if (ctype_digit($row[$i])) {
        for ($j = 0; $j < $row[$i]; $j++) {
          $v[$fileName] = [ 'piece' => '' ];
          $fileName++;
        }
      } else {
        $side = ctype_upper($row[$i]) ? 'w' : 'b';
        $piece = strtolower($row[$i]);
        $v[$fileName] = [ 'piece' => $side . $piece ];
        $fileName++;
      }
    }
    $board[$rankNumber--] = $v;
  }
  if ($epSquare != '-') {
    $board[$epSquare[1]][$epSquare[0]]['piece'] = 'epSquare';
  }
  return [ $board, $stm ];
}

function colorBoard(&$board) {
  foreach ($board as $i => $row) {
    foreach ($row as $j => $square) {
      $board[$i][$j]['color'] = (($i + ord($j)) % 2) ? 'whiteBg' : 'blackBg';
    }
  }
}

function serverQuery($fen) {
  $sock = @fsockopen('localhost', BACKEND_PORT);
  if (!$sock) {
    die('Backend server not responding');
  }
  fprintf($sock, "query $fen\n");
  $error = trim(fgets($sock));
  if ($error) {
    return [ null, null, null, $error ];
  }
  list($score, $numMoves) = fscanf($sock, '%s %d');
  $children = [];
  for ($i = 0; $i < $numMoves; $i++) {
    $line = fgets($sock);
    list($move, $childScore, $childFen) = explode(' ', $line, 3);
    $children[] = [
      'move' => $move,
      'score' => is_numeric($childScore) ? (int)$childScore : '',
      'scoreText' => $childScore,
      'fen' => $childFen,
    ];
  }
  fclose($sock);

  if (count($children) && is_numeric($children[0]['score'])) {
    usort($children, 'cmpChildren');
  }
  return [
    is_integer($score) ? $score : '',
    $score,
    $children,
    null,
  ];
}

function cmpChildren($a, $b) {
  $sa = sign($a['score']);
  $sb = sign($b['score']);
  if ($sa != $sb) {
    return ($sa < $sb) ? -1 : 1;
  }
  if ($a['score'] != $b['score']) {
    return ($a['score'] < $b['score']) ? 1 : -1;
  }
  return ($a['move'] < $b['move']) ? -1 : 1;
}

/* Returns -1 / 0 / 1 for an integer argument */
function sign($int) {
  return min(1, max(-1, $int));
}

function interpretScore($text) {
  if (is_numeric($text)) {
    return [ 'dtc' => $text, 'text' => '' ];
  } else {
    return [ 'dtc' => '', 'text' => $text ];
  }
}

?>
