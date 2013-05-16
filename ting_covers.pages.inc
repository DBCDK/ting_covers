<?php

/**
 * Page callback
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

    $path = ting_covers_object_path($local_id, 'medium');

    if ( file_exists($path) ) {
      //If we we already have a valid cover image w/o style then just use it
      // todo: put styles in admin? these are just the defaults in the image module.
      $covers[ $local_id['id'] . ':medium' ] = image_style_url('medium', $path);
      $covers[ $local_id['id'] . ':large' ]  = image_style_url('large', $path);
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
      if ( $file = _ting_covers_pages_fetch_image(ting_covers_object_path($local_id['id'], 'frontcover_jpg'), $source_url) ) {
        // Generate a path corresponding to the downloaded image, styled
        // todo: put styles in admin? these are just the defaults in the image module.
        $covers[ $local_id['id'] . ':medium' ] = image_style_url('medium', $file->uri);
        $covers[ $local_id['id'] . ':large' ]  = image_style_url('large', $file->uri);
      }

      if ( $file = _ting_covers_pages_fetch_image(ting_covers_object_path($local_id['id'], 'backcover_pdf'), $backcover_pdf_url) ) {
        //Generate a path corresponding to the downloaded image
        $covers[ $local_id['id'] . ':' . 'backcover_pdf' ] = $GLOBALS['base_url'] . '/sites/default/files/ting/covers/backcover_pdf/' . md5($local_id['id']) . '.pdf';
      }

    }
  } catch (Exception $e) {
    watchdog('ting_covers', 'Unable to retrieve covers from ADDI: %message', array('%message' => $e->getMessage()), WATCHDOG_ERROR);
  }


  // Return all image information
  // ex. 29316627:medium => http://domain.dk/path/to/covers/medium/md5id.jpg
  // ex. 29316627:backcover_pdf => http://jgn.ding.dbc.dk/path/to/covers/backcover_pdf/b6c14dd971a9158b38d7492a557616d7.pdf
  drupal_json_output($covers);
}


/**
 * page callback
 */
 function ting_covers_moreinfo_cover($local_id = NULL, $alt = ''){

    if ( !$local_id ) {
      return t('image ID is missing', array(), array('context' => 'ting_covers:error'));
    }

    $path = ting_covers_object_path($local_id, 'frontcover_jpg');

    if ( file_exists($path) ) {
      $output = theme('image_style', array('style_name' => 'large', 'path' => $path, 'alt' => $alt));
      return $output;
    } else {
      return t("image \"@lid\" not found.", array('@lid' => $local_id), array('context' => 'ting_covers:error'));
    }
}


/**
 * page callback
 */
 function ting_covers_moreinfo_backcover($local_id = NULL){

    if ( !$local_id ) {
      return t('pdf ID is missing', array(), array('context' => 'ting_covers:error'));
    }

    $path = ting_covers_object_path($local_id, 'backcover_pdf');

    if ( file_exists($path) ) {
      $src = $GLOBALS['base_url'] . '/sites/default/files/ting/covers/backcover_pdf/' . md5($local_id) . '.pdf';
      $output = '<object data="' . $src . '?page=1&amp;view=Fit" type="application/pdf" width="100%" height="96%">
        <p>It appears you don\'t have a PDF plugin for this browser.
           No biggie... you can <a href="' . $src . '">click here to download the PDF file.</a></p>
      </object>';
      return $output;
    } else {
      return t("pdf \"@lid\" not found.", array('@lid' => $local_id), array('context' => 'ting_covers:error'));
    }
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
    $file->uri =  'http://domain.dk/path/to/covers/medium/md5id.jpg';
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
