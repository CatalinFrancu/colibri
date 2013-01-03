<?php

require_once 'smarty/Smarty.class.php';

$board = array();
for ($i = 1; $i <= 8; $i++) {
  for ($j = 1; $j <= 8; $j++) {
    $board[$i][$j] = array('color' => (($i + $j) % 2) ? 'blackBg' : 'whiteBg',
                            'piece' => '');
  }
}

$board[3][2]['piece'] = 'wk';
$board[4][6]['piece'] = 'wb';
$board[6][4]['piece'] = 'bn';
$board[7][8]['piece'] = 'br';

$smarty = new Smarty();
$smarty->template_dir = 'templates';
$smarty->compile_dir = 'templates_c';
$smarty->assign('board', $board);
$smarty->display('index.tpl');

?>
