<?php

require_once __DIR__ . '/../resources/Core.php';

$smarty = Core::getSmarty();
$smarty->assign('template', 'about.tpl');
$smarty->display('layout.tpl');

?>
