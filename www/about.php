<?php

require_once 'smarty3/Smarty.class.php';

$smarty = new Smarty();
$smarty->template_dir = 'templates';
$smarty->compile_dir = 'templates_c';
$smarty->assign('template', 'about.tpl');
$smarty->display('layout.tpl');

?>
