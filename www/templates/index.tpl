<!DOCTYPE html>
<html>
  <head>
    <title>Colibri Suicide Chess Browser</title>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
    <link href="css/main.css?v=1" rel="stylesheet" type="text/css"/>
    <script src="js/jquery-1.8.3.min.js"></script>
    <script src="js/main.js?v=1"></script>
  </head>

  <body>
    <div id="leftColumn">
      <table id="board">
        <tr>
          <th class="lr tb"></th>
          {foreach from=$board[8] item=square key=fileName}
          <th class="tb">{$fileName}</th>
          {/foreach}
          <th class="lr tb {if $stm == 'w'}stmOff{else}stmOn{/if}"></th>
        </tr>
        {foreach from=$board item=rank key=rankNumber}
          <tr>
            <th class="lr">{$rankNumber}</th>
            {foreach from=$rank item=square}
              <td class="square anvil {$square.color}"><div class="{$square.piece}"></div></td>
            {/foreach}
            <th class="lr">{$rankNumber}</th>
          </tr>
        {/foreach}
        <tr>
          <th class="lr tb"></th>
          {foreach from=$board[8] item=square key=fileName}
          <th class="tb">{$fileName}</th>
          {/foreach}
          <th class="lr tb {if $stm == 'w'}stmOn{else}stmOff{/if}"></th>
        </tr>
      </table>

      <div>
        <a id="editLink" href="#">edit position</a>
      </div>

      <div id="toolBox">
        Click on a piece, then click on a board square to place it.
        <table id="editBox">
          <tr>
            <td class="square hammer"><div class="wk"></div></td>
            <td class="square hammer"><div class="wq"></div></td>
            <td class="square hammer"><div class="wr"></div></td>
            <td class="square hammer"><div class="wb"></div></td>
            <td class="square hammer"><div class="wn"></div></td>
            <td class="square hammer"><div class="wp"></div></td>
            <td class="square hammer eraseOne" title="Erase one piece"><div></div></td>
          </tr>
          <tr>
            <td class="square hammer"><div class="bk"></div></td>
            <td class="square hammer"><div class="bq"></div></td>
            <td class="square hammer"><div class="br"></div></td>
            <td class="square hammer"><div class="bb"></div></td>
            <td class="square hammer"><div class="bn"></div></td>
            <td class="square hammer"><div class="bp"></div></td>
            <td class="square hammer eraseAll" title="Erase the entire board"><div></div></td>
          </tr>
        </table>

        <div class="stmHeader">Side to move:</div>
        <div class="stm stmWhite {if $stm == 'w'}stmSelected{/if}">white</div>
        <div class="stm stmBlack {if $stm == 'b'}stmSelected{/if}">black</div>
        <form id="editForm" action="index.php">
          <input id="fen" type="hidden" name="fen" value=""/>
          <input id="goButton" type="button" value="Go!"/>
        </form>

        <div style="clear: both"></div>
      </div>
    </div>

    <div id="rightColumn">
      <table id="moves">
        {foreach from=$moves item=m}
          <tr>
            <td>
              <a href="?fen={$m.fen|escape}">{$m.move}</a>
            </td>
            <td class="{include file=scoreClass.tpl score=$m.score}">{include file=score.tpl score=$m.score}</td>
          </tr>
        {/foreach}
      </table>
    </div>

    <script>
      currentStm = '{$stm}';
    </script>
  </body>
</html>
