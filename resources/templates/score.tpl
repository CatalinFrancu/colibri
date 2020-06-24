{* from the parent's perspective *}
{if $score < 0}
  wins in {-$score}
{elseif $score > 0}
  loses in {$score}
{elseif $score === 0}
  draw
{else}
  {$scoreText}
{/if}
