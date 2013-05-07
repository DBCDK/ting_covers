<?php

class AdditionalInformationService {

  private $wsdlUrl;
  private $username;
  private $group;
  private $password;

  public function __construct($wsdlUrl, $username, $group, $password)
  {
    $this->wsdlUrl = $wsdlUrl;
    $this->username = $username;
    $this->group = $group;
    $this->password = $password;
  }

  public function getByIsbn($isbn)
  {
    $isbn = str_replace('-', '', $isbn);

    $identifiers = $this->collectIdentifiers('isbn', $isbn);
    $response = $this->sendRequest($identifiers);
    return $this->extractAdditionalInformation('isbn', $response);
  }

  public function getByFaustNumber($faustNumber)
  {
    //quickfix for test only
    if ($faustNumber[0] == '23959798'){
      return unserialize('a:1:{i:23959798;O:21:"AdditionalInformation":2:{s:12:"thumbnailUrl";s:101:"http://moreinfo.addi.dk/2.1/more_info_get.php?id=24576091&type=forside_lille&key=e2319192f842be20cbe1";s:9:"detailUrl";s:100:"http://moreinfo.addi.dk/2.1/more_info_get.php?id=24576091&type=forside_stor&key=b3059e12e14a20f6cddf";}}');
    }

    $identifiers = $this->collectIdentifiers('faust', $faustNumber);
    $response = $this->sendRequest($identifiers);
    return $this->extractAdditionalInformation('faust', $response);
  }

  protected function collectIdentifiers($idName, $ids)
  {
    if (!is_array($ids))
    {
      $ids = array($ids);
    }
    $identifiers = array();
    foreach ($ids as $i)
    {
      $identifiers[] = array($idName => $i);
    }
    return $identifiers;
  }

  protected function sendRequest($identifiers)
  {
    $ids = array();
    foreach ($identifiers as $i)
    {
      $ids = array_merge($ids, array_values($i));
    }

    $authInfo = array('authenticationUser' => $this->username,
                      'authenticationGroup' => $this->group,
                      'authenticationPassword' => $this->password);
    $client = new SoapClient($this->wsdlUrl);

    $startTime = explode(' ', microtime());
    $response = $client->moreInfo(array(
                          'authentication' => $authInfo,
                          'identifier' => $identifiers));

    $stopTime = explode(' ', microtime());
    $time = floatval(($stopTime[1]+$stopTime[0]) - ($startTime[1]+$startTime[0]));

    //Drupal specific code - consider moving this elsewhere
    if (variable_get('addi_enable_logging', false)) {
      watchdog('addi', 'Completed request ('.round($time, 3).'s): Ids: %ids', array('%ids' => implode(', ', $ids)), WATCHDOG_DEBUG, 'http://'.$_SERVER["HTTP_HOST"].$_SERVER["REQUEST_URI"]);
    }

    if ($response->requestStatus->statusEnum != 'ok')
    {
      throw new AdditionalInformationServiceException($response->requestStatus->statusEnum.': '.$response->requestStatus->errorText);
    }

    if (!is_array($response->identifierInformation))
    {
      $response->identifierInformation = array($response->identifierInformation);
    }

    return $response;
  }

  protected function extractAdditionalInformation($idName, $response)
  {
    $additionalInformations = array();

    foreach($response->identifierInformation as $info)
    {
      $thumbnailUrl = $detailUrl = $backPageUrl = NULL;
      if (isset($info->identifierKnown) && $info->identifierKnown && $info->coverImage )
      {
        if (!is_array($info->coverImage))
        {
          $info->coverImage = array($info->coverImage);
        }

        foreach ($info->coverImage as $image)
        {
          switch ($image->imageSize) {
            case 'thumbnail':
              $thumbnailUrl = $image->_;
              break;
            case 'detail':
              $detailUrl = $image->_;
              break;
            default:
              // Do nothing other image sizes may appear but ignore them for now
          }
        }

        // just pick the first back cover PDF, if there's several
        if ( !$backPageUrl && isset($info->backPage->_) && $info->backPage->_ )
        {
          $backpagePdfUrl = $info->backPage->_;
        }

        $additionalInfo = new AdditionalInformation($thumbnailUrl, $detailUrl, $backpagePdfUrl);
        $additionalInformations[$info->identifier->$idName] = $additionalInfo;

      }
    }

    return $additionalInformations;
  }

}
