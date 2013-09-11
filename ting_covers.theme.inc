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
  $alt = '';
  $modal_link = '';

  //Set initial values required by the template, and check if there's an image available locally
  $path = FALSE;
  $backcoverpdf_path = $backcoverpdf_local_id = FALSE;
  switch ($object->ding_entity_type) {
    case 'bibdk_entity': // bibdkWork object
      $classes = array('ting-cover', 'ting-cover-style-'.$image_style);
      foreach ($work_local_ids as $key => $lid) {
        if ( !$path ) { // we'll just use the first available local image.
          if ( $path = ting_covers_object_path($lid, 'frontcover_jpg') ) {
            $local_id = $lid;
          }
        }
        if ( !$backcoverpdf_path ) { // ditto back cover pdf
          if ( $backcoverpdf_path = ting_covers_object_path($lid, 'backcover_pdf') ) {
            $backcoverpdf_local_id = $lid;
          }
        }
        $classes[] = 'ting-cover-work-object-id-'.$lid;
        // we don't know which local_id will be used, so we'll have to activate a modal path for all of them
        $modal_link .= l(
          '',
          'ting/moreinfo/covers/' . $lid . '/foobar',
          array(
            'attributes' => array('class'=>array('visuallyhidden', 'id-' . $lid), 'rel' => 'lightframe[|width:350px; height:500px; scrolling: auto;]')
          )
        );
      }
      break;
    case 'bibdk_manifestation':
    default: // Ting object
      $path = ting_covers_object_path($local_id, 'frontcover_jpg');
      $backcoverpdf_path = ting_covers_object_path($local_id, 'backcover_pdf');
      $classes = array('ting-cover',
                       'ting-cover-object-id-'.$local_id,
                       'ting-cover-style-'.$image_style);
      // Activate a modal path for this local_id (it's actually inserted by jQuery)
      $modal_link .= l(
        '',
        'ting/moreinfo/covers/' . $lid . '/foobar',
        array(
          'attributes' => array('class'=>array('visuallyhidden', 'id-' . $local_id), 'rel' => 'lightframe[|width:350; height:550px; scrolling: auto;]')
        )
      );
      break;
  }

  $variables['elements']['#classes'] = $classes;

  $variables['elements']['#image'] = $modal_link;

  $variables['elements']['#no_image_class'] = ' visuallyhidden';

  //If we already have the image available locally then just use it
  if ( file_exists($path) ) {

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

    $alt = ( !empty($alt) ) ? $alt : 'foobar';
    $variables['elements']['#image'] = l(
      theme('image_style', array('style_name' => $image_style, 'path' => $path, 'alt' => $alt)),
      'ting/moreinfo/covers/' . $local_id . '/' . $alt,
      array(
        'html' => TRUE,
        'attributes' => array(
          'rel' => 'lightframe[|width:350; height:550px; scrolling: auto;]',
        )
      )
    );

    $variables['elements']['#no_image_class'] = '';

    //Avoid further javascript processing
    $variables['elements']['#classes'][] = 'ting-cover-processed';

  } elseif (cache_get('ting_covers:' . $local_id)) {

    //We know that there is no cover available for this object so avoid
    //further javascript processing
    $variables['elements']['#classes'][] = 'ting-cover-processed';

  }

  drupal_add_js(drupal_get_path('module', 'ting_covers') . '/js/ting-covers.js');


  //front cover link:
  $options = array(
    'attributes' => array(
      'class' => array('work-cover-front', 'active'),
      'rel' => 'lightframe[|width:350; height:550px; scrolling: auto;]',
    )
  );
  if ( !file_exists($backcoverpdf_path) ) {
    $options['attributes']['class'][] = 'visuallyhidden';
    $options['attributes']['disabled'] = 'disabled';

  }
    $options['attributes']['title'] = t('Forside');


  $variables['elements']['#front_cover_large_link'] = l(
    '',
    'ting/moreinfo/covers/' . $local_id . '/' . $alt,
    $options
  );


  // ditto back cover pdf:
  $options = array(
    'attributes' => array(
      'class' => array('work-cover-back'),
      'rel' => 'lightframe[|width:600px; height:900px; scrolling: auto;]',
    ),
    'html' => FALSE
  );
  if ( !file_exists($backcoverpdf_path) ) {
    $options['attributes']['class'][] = 'visuallyhidden';
    $options['attributes']['disabled'] = 'disabled';

  }
    $options['attributes']['title'] = t('Bagside');

  $variables['elements']['#back_cover_large_link'] = l(
    '',
    'ting/moreinfo/back_covers/' . $backcoverpdf_local_id,
    $options
  );
}
