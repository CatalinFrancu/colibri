{strip}
  {if $score > 0}
    scoreWin
  {elseif $score < 0}
    scoreLose
  {else}
    scoreDraw
  {/if}
{/strip}
