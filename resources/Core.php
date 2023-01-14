<?php

require_once __DIR__ . '/smarty-4.3.0/Smarty.class.php';

class Core {

  private static $smarty = null;

  static function getSmarty() {
    if (!self::$smarty) {
      self::$smarty = new Smarty();
      self::$smarty->template_dir = __DIR__ . '/templates';
      self::$smarty->compile_dir = __DIR__ . '/templates_c';
      return self::$smarty;
    }
  }
}
