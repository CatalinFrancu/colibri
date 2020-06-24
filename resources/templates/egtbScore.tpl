{* convert colibri scores to human-readable scores *}
{if $score == 0}
  draw
{else}
  {$hscore=($score>0)?($score-1):(-$score-1)}

  {if $score > 0}
    <span class="text-success">win</span>
  {else}
    <span class="text-danger">loss</span>
  {/if}

  {if $hscore == 0}
    now
  {else}
    in {$hscore} half-moves
  {/if}
{/if}
