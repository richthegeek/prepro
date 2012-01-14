<?php



function prepro_admin_form($form_id, &$form_state) {
  $types = _prepro_get_types();
  $settings = _prepro_get_settings();
  $selected = array(); # keep track of chosen preprocs for deciding wether a fieldset is open by default

  $rows = array();
  foreach ($types as $format => $processors) {
    $proc_list = array();
    foreach ($processors as $name => $proc) {
      $proc_list[$name] = $proc['label'];
    }
    $settings['filetypes'] += array(
      $format => array(
        'preprocessor' => current(array_keys($proc_list)),
        'cache' => 'cache',
      ),
    );
    $selected[] = $settings['filetypes'][$format]['preprocessor'];
    $rows[] = array(
      'name' => '.' . $format,
      'processor' => count($proc_list) == 1
        ? $proc['label']
        : array('data' => array(
          '#name' => 'filetypes[' . $format . '][preprocessor]',
          '#type' => 'select',
          '#options' => $proc_list,
          '#value' => $settings['filetypes'][$format]['preprocessor'],
        )
      ),
      'cache' => array('data' => array(
        '#name' => 'filetypes[' . $format . '][cache]',
        '#type' => 'select',
        '#options' => array(
          'cache' => 'Cached, reprocessed on cache-clear',
          'onload' => 'Uncached, reprocessed on every page load',
        ),
        '#value' => $settings['filetypes'][$format]['cache'],
      )),
    );
  }

  $form = array();
  $form['filetypes'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filetype-specific settings'),
    'table' => array(
      '#theme' => 'table',
      '#header' => array('Format', 'Pre-processor', 'Caching'),
      '#rows' => $rows,
    )
  );

  foreach (module_implements('css_preprocessor_settings_form') as $module) {
    $procs = module_invoke($module, 'css_preprocessor_info');
    $form[$module] = array();
    foreach ($procs as $proc => $info) {
      $settings['additional'] += array($proc => array());
      $result = module_invoke($module, 'css_preprocessor_settings_form', $settings['additional'][$proc], $settings, $proc);
      if ($result) {
        _prepro_admin_form_alter_name($result, $proc);
        $form[$module][$proc] = array(
          '#type' => 'fieldset',
          '#collapsible' => TRUE,
          '#collapsed' => !in_array($proc, $selected),
          '#title' => $info['label'],
          '#name' => $proc,
          'fields' => $result
        );
      }
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save Settings',
  );

  return $form;
}

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

function prepro_admin_form_submit($form, $form_state) {
  $settings = _prepro_get_settings(array(
    'additional' => $form_state['input']['additional'],
    'filetypes' => $form_state['input']['filetypes']
  ));
  variable_set('prepro', $settings);
  drupal_set_message('Prepro settings have been saved');
}
