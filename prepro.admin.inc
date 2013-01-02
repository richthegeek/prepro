<?php
/**
 * @file
 */

/**
 * Callback for Prepro settings page - compiles a table of filetype-specific settings
 * and any preprocessor-specific settings available.
 */
function prepro_admin_form($form_id, &$form_state) {
  $compilers = prepro_get_compilers();
  $cachers = prepro_get_cachers();
  $settings = prepro_get_settings();
  $selected = array(); // keep track of chosen preprocs for deciding wether a fieldset is open by default
  $rows = array();

  foreach ($cachers as $key => $cacher) {
    $cache_options[$cacher['module'] . '/' . $key] = $cacher['title'] . ': ' . $cacher['description'];
  }

  foreach ($compilers as $format => $processors) {
    $proc_list = array();
    foreach ($processors as $key => $proc) {
      $proc_list[$proc['module'] . '/' . $key] = $proc['label'];
    }
    $settings['filetypes'] += array($format => array());
    $settings['filetypes'][$format] += array(
      'compile_handler' => current(array_keys($proc_list)),
      'cache_handler' => '',
      'cache_path' => 'public://prepro/'
    );
    $selected[] = $settings['filetypes'][$format]['compile_handler'];
    $rows[] = array(
      'name' => '.' . $format,
      'compile_handler' => array('data' => array(
        '#name' => 'filetypes[' . $format . '][compile_handler]',
        '#type' => 'select',
        '#options' => $proc_list,
        '#value' => $settings['filetypes'][$format]['compile_handler'],
      )),
      'cache_handler' => array('data' => array(
        '#name' => 'filetypes[' . $format . '][cache_handler]',
        '#type' => 'select',
        '#options' => $cache_options,
        '#value' => $settings['filetypes'][$format]['cache_handler'],
      )),
      'cache_path' => array('data' => array(
        '#name' => 'filetypes[' . $format . '][cache_path]',
        '#type' => 'textfield',
        '#value' => rtrim($settings['filetypes'][$format]['cache_path'], '/')
      )),
    );
  }

  $form = array();
  $form['filetypes'] = array(
    '#theme' => 'table',
    '#header' => array('Format', 'Pre-processor', 'Caching', 'Cache Location'),
    '#rows' => $rows,
    '#empty' => 'There are no modules offering to preprocess JS/CSS files'
  );

  foreach (module_implements('preprocessor_settings_form') as $module) {
    $procs = module_invoke($module, 'preprocessor_info');
    $form[$module] = array();
    foreach ($procs as $proc => $info) {
      // use the "retrieve" and "prepare" funcs to get the form and allow the usual form handler functions
      $form_id = $module . '_preprocessor_settings_form';
      $settings['additional'] += array($proc => array());
      $temp_state = array('prepro' => array('local' => $settings['additional'][$proc], 'global' => $settings, 'processor' => $proc)) + form_state_defaults();
      $result = drupal_retrieve_form($form_id, $temp_state);
      // if we got the form, alter the form element names to make saving v simple.
      if ($result) {
        _prepro_admin_form_alter_name($result, $proc);
      }
      else {
        // otherwise, remove the compiler from the "selected" list
        foreach ($selected as $i => $v) {
          if ($v == $proc) {
            unset($selected[$i]);
          }
        }
        // and change the label a wee bit.
        $info['label'] .= ' (no settings)';
        $result = array();//
      }

      // add the result to the page.
      $form[$module][$proc] = array(
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => !in_array($proc, $selected),
        '#title' => filter_xss($info['label']),
        '#name' => $proc,
        'fields' => $result
      );
    }
  }

  $form['buttons'] = array(
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Save settings',
      '#submit' => array('prepro_admin_form_save')
    ),
    'clear_cache' => array(
      '#type' => 'submit',
      '#value' => 'Clear prepro cache',
      '#submit' => array('prepro_admin_form_cc')
    ),
  );

  return $form;
}

/**
 * Helper: alters the #name attribute of all inputs to make them specific
 * to the preprocessor (for example 'hello' to 'additional[sassy][hello]')
 */
function _prepro_admin_form_alter_name(&$array, $proc) {
  foreach ($array as $key => $value) {
    if ($key{0} != '#') {
      if (!isset($value['#name'])) {
        $value['#name'] = 'additional[' . $proc . '][' . $key . ']';
      }
      else {
        $value['#name'] = preg_replace('/^([^\[]+)(\[.+)?/', 'additional[' . $proc . '][$1]$2', $value['#name']);
      }
      if (is_array($value)) {
        _prepro_admin_form_alter_name($value, $proc);
      }
    }
    $array[$key] = $value;
  }
}

/**
 * Submit handler for "Save" on the prepro admin form.
 */
function prepro_admin_form_save($form, &$form_state) {
  $settings = array(
    'additional' => $form_state['input']['additional'],
    'filetypes' => $form_state['input']['filetypes']
  );
  // make sure that we can write to all the cache locations.
  foreach ($settings['filetypes'] as $ext => $type) {
    if (!file_prepare_directory($type['cache_path'], FILE_MODIFY_PERMISSIONS ^ FILE_CREATE_DIRECTORY)) {
      drupal_set_message(t('The cache location for :ext files (:dir) can either not be created or is not writable.', array(':ext' => $ext, ':dir' => $type['cache_path'])), 'warning');
    }
  }
  variable_set('prepro', $settings);
  drupal_set_message(t('Prepro settings have been saved'));
  prepro_admin_form_cc($form, $form_state);
}

/**
 * Submit handler for clearing the cache from the prepro admin form
 */
function prepro_admin_form_cc($form, &$form_state) {
  cache_clear_all('prepro:map', 'cache_prepro');
  drupal_set_message(t('The Prepro cache has been cleared'));
}


/**
 * Implements hook_form_FORM_ID_alter().
 */
function prepro_form_system_performance_settings_alter(&$form) {
  $form['clear_cache']['prepro_clear'] = array(
    '#type' => 'submit',
    '#value' => 'Clear prepro cache',
    '#submit' => array('prepro_admin_form_cc')
  );
}
