  <div class="work-cover">
    <div class="work-cover-image<?php print $elements['#no_image_class'] ?>">

      <div class="<?php print( implode(' ', $elements['#classes'])); ?>">
        <?php print $elements['#image'] ?>
      </div>

<!--
      <a href="#"><img src="../img/cover-front-large.gif"></a>
      <a href="#" class="visuallyhidden"><img src="../img/cover-back-large.gif"></a>
-->

    </div>
    <div class="work-cover-selector clearfix<?php print $elements['#no_backcoverpdf_class'] ?>">
      <a href="#" class="work-cover-front active"></a>
      <a href="<?php print $elements['#backcoverpdf_url'] ?>" class="work-cover-back bibdk-popup-link" rel="backcoverpdf"></a>
    </div>
  </div>

<!--
<div class="<?php print( implode(' ', $elements['#classes'])); ?>">
  <?php print $elements['#image'] ?>
</div>
-->
