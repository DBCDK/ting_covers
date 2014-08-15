<?php
/**
 * @file
 * Renders the HTML for a work cover
 */
?>

<div class="work-cover">

  <div class="work-cover__image">
    <div class="<?php print( implode(' ', $elements['#classes'])); ?>">
      <?php print $elements['#image'] ?>
    </div>
  </div>

  <div class="work-cover__selectors">
    <?php print $elements['#front_cover_large_link'] ?>
    <?php print $elements['#back_cover_large_link'] ?>
  </div>

</div>
