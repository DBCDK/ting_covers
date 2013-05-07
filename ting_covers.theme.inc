<?php

function template_preprocess_ting_object_cover(&$variables) {

  $object = $variables['elements']['#object'];

  switch ($object->ding_entity_type) {
    case 'bibdk_entity': // bibdkWork object
      foreach ($object->manifestations as $key => $manifestation) {
        $local_id = explode(':',$manifestation->ding_entity_id);
        $work_local_ids[] = $local_id[1];
      }
      break;
    case 'bibdk_manifestation':
      $local_id = explode(':',$object->ding_entity_id);
      $local_id = $local_id[1];
      break;
    default: // Ting object
      $local_id = $object->localId;
      break;
  }

  $image_style = $variables['elements']['#image_style'];

  //Set initial values required by the template, and check if there's an image available locally
  $path = FALSE;
  $backcoverpdf_path = $backcoverpdf_local_id = FALSE;
  switch ($object->ding_entity_type) {
    case 'bibdk_entity': // bibdkWork object
      $classes = array('ting-cover', 'ting-cover-style-'.$image_style);
      foreach ($work_local_ids as $key => $local_id) {
        if ( !$path ) { // we'll just use the first available local image.
          $path = ting_covers_object_path($local_id, $image_style);
        }
        if ( !$backcoverpdf_path ) { // ditto back cover pdf
          $backcoverpdf_path = ting_covers_object_path($local_id, 'backcover_pdf');
          if ( $backcoverpdf_path ) {
            $backcoverpdf_local_id = $local_id;
          }
        }
        $classes[] = 'ting-cover-work-object-id-'.$local_id;
      }
      break;
    case 'bibdk_manifestation':
    default: // Ting object
      $path = ting_covers_object_path($local_id, $image_style);
      $backcoverpdf_path = ting_covers_object_path($local_id, 'backcover_pdf');
      $classes = array('ting-cover',
                       'ting-cover-object-id-'.$local_id,
                       'ting-cover-style-'.$image_style);
      break;
  }

  $variables['elements']['#classes'] = $classes;

  $variables['elements']['#image'] = '';
  $variables['elements']['#no_image_class'] = ' visuallyhidden';

  //If we already have the image available locally then just use it
  if (file_exists($path)) {
    //Generate an alt tag
    switch ($object->ding_entity_type) {
      case 'bibdk_entity': // bibdkWork object
        if ( isset($object->work->creators) ) {
          if ( is_array($object->work->creators->creator) ) {
            foreach ($object->work->creators->creator as $key => $creator) {
              $creators[] = $creator->display->{'$'};
            }
            $alt = implode(', ', $creators);
          } else {
            $alt = $object->work->creators->creator->display->{'$'};
          }
          $alt .= ': ' . $object->work->title->titleSpecific->{'$'};
        } else {
          $alt = $object->work->title->titleSpecific->{'$'};
        }
        break;
      case 'bibdk_manifestation':
        if ( isset($object->manifestation->work->creators) ) {
          if ( is_array($object->manifestation->work->creators->creator) ) {
            foreach ($object->manifestation->work->creators->creator as $key => $creator) {
              $creators[] = $creator->display->{'$'};
            }
            $alt = implode(', ', $creators);
          } else {
            $alt = $object->manifestation->work->creators->creator->display->{'$'};
          }
          $alt .= ': ' . $object->manifestation->work->title->titleSpecific->{'$'};
        } else {
          $alt = $object->manifestation->work->title->titleSpecific->{'$'};
        }
        break;
      default: // Ting object
        $alt = implode(', ', $object->creators).': '.$object->title;
        break;
    }

    $variables['elements']['#image'] = theme('image_style', array('style_name' => $image_style, 'path' => $path, 'alt' => $alt));
    $variables['elements']['#no_image_class'] = '';

    //Avoid further javascript processing
    $variables['elements']['#classes'][] = 'ting-cover-processed';
  } elseif (cache_get('ting_covers:' . $local_id)) {
    //We know that there is no cover available for this object so avoid
    //further javascript processing
    $variables['elements']['#classes'][] = 'ting-cover-processed';
  }

  drupal_add_js(drupal_get_path('module', 'ting_covers') . '/js/ting-covers.js');

  // ditto back cover pdf
  $variables['elements']['#backcoverpdf_url'] = '#';
  $variables['elements']['#no_backcoverpdf_class'] = ' visuallyhidden';
  if ( file_exists($backcoverpdf_path) ) {
    $variables['elements']['#backcoverpdf_url'] = $GLOBALS['base_url'] . '/sites/default/files/ting/covers/object/backcover_pdf/' . md5($backcoverpdf_local_id) . '.pdf';;
    $variables['elements']['#no_backcoverpdf_class'] = '';
  }

}

