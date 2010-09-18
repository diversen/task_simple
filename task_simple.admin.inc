<?php

//$Id$ 

/*
 * file for admin functions
 */

/*
 * function task_simple_admin
 * @returns system_settings_form($form)
 */
function task_simple_admin() {
  $form['task_simple_pager'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of links'),
    '#default_value' => variable_get('task_simple_pager', 5),
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t("Tasks per page"),
    '#required' => TRUE,
  );
  
  $form['task_simple_timezone'] = array(
    '#type' => 'textfield',
    '#title' => t('Use users locale timezone if set'),
    '#default_value' => variable_get('task_simple_timezone', 1),
    '#size' => 1,
    '#maxlength' => 1,
    '#description' => t("Use users local timezone (1) or system default (0)"),
    '#required' => TRUE,
  );
  return system_settings_form($form);
}

/*
 * function task_simple_admin_validate
 * @param $form
 * @param &$form_state
 */
function task_simple_admin_validate($form, &$form_state) {
  $maxdisp = $form_state['values']['task_simple_pager'];
  $timezone = $form_state['values']['task_simple_timezone'];
  
  if (!is_numeric($maxdisp)) {
    form_set_error('task_simple_pager', t('You must select a number for max tasks per page'));
  }
  else if ($maxdisp <= 0) {
    form_set_error('task_simple_pager', t('Maximum number of tasks per page must be positive.'));
  }
  if (!is_numeric($timezone)) {
    form_set_error('task_simple_timezone', t('Timezone must be 1 or 0'));
  } else if ($timezone < 0 || $timezone > 1) {
    form_set_error('task_simple_timezone', t('Timezone must be 1 or 0'));
  }
}

