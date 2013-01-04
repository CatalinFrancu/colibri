{if $score > 0}
  wins in {$score}
{elseif $score < 0}
  loses in {math equation="-x" x=$score}
{else}
  draw
{/if}
