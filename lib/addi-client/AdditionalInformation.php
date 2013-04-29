<?php

class AdditionalInformation {

  public $thumbnailUrl;
  public $detailUrl;
  public $backpageUrl;

  public function __construct($thumbnailUrl, $detailUrl, $backpageUrl)
  {
    $this->thumbnailUrl = $thumbnailUrl;
    $this->detailUrl = $detailUrl;
    $this->backpageUrl = $backpageUrl;
  }

}