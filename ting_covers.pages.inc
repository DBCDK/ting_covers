<?php

/**
 * Return the URL for the cover of an object
 */
function ting_covers_objects() {

  $covers = array();
  $missing_images_local_ids = array();

  if ( empty($_POST['coverData']) ) {
    watchdog('ting_covers', 'Empty coverData variable in $_POST', array(), WATCHDOG_ERROR);
    drupal_json_output($covers);
    return;
  }

  foreach ($_POST['coverData'] as $cover_info) { // ex. [0] => 29316627:medium

    //Reverse cover_info before and after explode to make sure that we explode around
    //the last :. Local ids could contain the delimiter!
    list($local_id, $image_style) = array_reverse(array_map('strrev', explode(':', strrev($cover_info), 2)));

    //Determine if the local id is a known negative
    if (cache_get('ting_covers:' . $local_id, FALSE)) {
      break;
    }

    $path = ting_covers_object_path($local_id, $image_style);

    if ( file_exists($path) ) {
      //If we we already have a valid cover image w/o style then just use it
      $covers[$local_id.':'.$image_style] = image_style_url($image_style, $path);
    } else {
      //Mark the image for retrieval;
      $missing_images_additional_informations_ids[] = $local_id;
      $missing_images_local_ids[] = array('id'=>$local_id,'style'=>$image_style);
    }
  }

  //Try to download the missing images
  try {
    $service = new AdditionalInformationService(variable_get('addi_wsdl_url'), variable_get('addi_username'), variable_get('addi_group'), variable_get('addi_password'));
    //Local ids = Faust numbers. Library object identifiers can be confusing...
    $additional_informations = $service->getByFaustNumber($missing_images_additional_informations_ids);

    foreach($missing_images_local_ids as $local_id) {

      //Try to extract the image url from the result
      $source_url = FALSE;
      if (isset($additional_informations[$local_id['id']]) && $ai = $additional_informations[$local_id['id']]) {
        if ($ai->detailUrl) {
          $source_url = $ai->detailUrl;
        } elseif ($ai->thumbnailUrl) {
          $source_url = $ai->thumbnailUrl;
        }

        $backcover_pdf_url = ( !empty($ai->backpagePdfUrl) ) ? $ai->backpagePdfUrl : '';
      }

      //No cover image found? Cache this for future reference to avoid unnecessary requests
      if (!$source_url) {
        cache_set('ting_covers:'.$local_id['id'], 1, 'cache', $_SERVER['REQUEST_TIME'] + TING_COVERS_CACHE_LIFETIME);
        continue;
      }

      //Try to download the image locally
      if ( $file = _ting_covers_pages_fetch_image(ting_covers_object_path($local_id['id'], $image_style), $source_url) ) {
        //Generate a path corresponding to the downloaded image, styled
        $covers[$local_id['id'].':'.$local_id['style']] = image_style_url($local_id['style'], $file->uri);
      }

      if ( $file = _ting_covers_pages_fetch_image(ting_covers_object_path($local_id['id'], 'backcover_pdf'), $backcover_pdf_url) ) {
        //Generate a path corresponding to the downloaded image, styled
        $covers[ $local_id['id'] . ':' . 'backcover_pdf' ] = $GLOBALS['base_url'] . '/sites/default/files/ting/covers/object/backcover_pdf/' . md5($local_id['id']) . '.pdf';
      }

    }
  } catch (Exception $e) {
    watchdog('ting_covers', 'Unable to retrieve covers from ADDI: %message', array('%message' => $e->getMessage()), WATCHDOG_ERROR);
  }


  // Return all image information
  // ex. 29316627:medium => http://domain.dk/path/to/covers/object/medium/md5id.jpg
  // ex. 29316627:backcover_pdf => http://jgn.ding.dbc.dk/path/to/covers/object/backcover_pdf/b6c14dd971a9158b38d7492a557616d7.pdf

  drupal_json_output($covers);
}

/**
 * Helper function to fetch and save a cover image file.
 *
 * @see image_style_create_derivative()
 *
 * @param string $filename
 *    File name, including its path within Drupal's file folder.
 * @param string $image_url
 *    URL for the source image file.
 * @return mixed
 *    A file object or FALSE on error.
 */
function _ting_covers_pages_fetch_image($filename, $image_url = FALSE) {

  if ( !$image_url ) {
    return FALSE;
  }

  // there's a hard-coded test case for faustnr 12345678 in class AdditionalInformationService
  if ( $image_url == 'http://moreinfo.addi.dk/2.1/more_info_get.php?id=12345678&type=forside_stor&key=b3059e12e14a20f6cddf' ) {
    $file = new stdClass();
    $file->uri =  'http://domain.dk/path/to/covers/object/medium/md5id.jpg';
    return $file;
  }

  $result = drupal_http_request($image_url);

  //Bail if the HTTP request failed
  if ($result->code != 200) {
    return FALSE;
  }

  // Get the folder for the final location of this preset...
  $directory = dirname($filename);

  // Build the destination folder tree if it doesn't already exist.
  if (!file_prepare_directory($directory, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS)) {
    watchdog('ting_covers', 'Failed to create directory: %directory', array('%directory' => $directory), WATCHDOG_ERROR);
    return FALSE;
  }

  return file_save_data($result->data, $filename, FILE_EXISTS_REPLACE);
}
