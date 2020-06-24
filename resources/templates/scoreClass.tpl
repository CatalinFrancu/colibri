{* from the parent's perspective *}
{strip}
  {if is_numeric($score) && ($score < 0)}
    scoreWin
  {elseif is_numeric($score) && ($score > 0)}
    scoreLose
  {else}
    scoreDraw
  {/if}
{/strip}
