<?php

class AdditionalInformation {

  public $thumbnailUrl;
  public $detailUrl;
  public $backpagePdfUrl;

  public function __construct($thumbnailUrl, $detailUrl, $backpagePdfUrl)
  {
    $this->thumbnailUrl = $thumbnailUrl;
    $this->detailUrl = $detailUrl;
    $this->backpagePdfUrl = $backpagePdfUrl;
  }

}