<?php

global $user;
if (!empty($user->timezone_name)){
  if (variable_get('task_simple_timezone', 0)){
    date_default_timezone_set($user->timezone_name);
  }
}
/* 
 * used for creating the select element of the forms
 * progress is an integer between 1 (start) and 5 (finish)
 * @return an array of progress to use in drop down boxes
 */
function task_simple_get_progress(){

  $ary = array();
  $ary[1] = t('0%');
  $ary[2] = t('25%'); 
  $ary[3] = t('50%'); 
  $ary[4] = t('75%'); 
  $ary[5] = t('100%');
  return $ary;
}

/* 
 * @param $form_state
 * @param $keys String (the search terms)
 * @return a form to filter tasks.
 */
function task_simple_filter_form(&$form_state, $keys = '') {
  $form['#attributes'] = array('class' => 'search-form');
  $form['basic'] = array('#type' => 'fieldset',
    '#title' => t('Search in titles')
  );

  $form['basic']['inline']['filter'] = array(
    '#type' => 'textfield',
    '#title' => t('Search title'),
    '#required' => TRUE,
    '#default_value' => $keys,
  );
  
  $form['basic']['inline']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Search'),
    '#submit' => array('task_simple_filter_form_submit_filter'),
    );
  
  if ($keys) {
    $form['basic']['inline']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
      '#submit' => array('task_simple_filter_form_submit_reset'),
    );
  }
  return $form;
}


/* 
 * Process filter form submission when the Filter button is pressed.
 */
function task_simple_filter_form_submit_filter($form, &$form_state) {
  $form_state['redirect'] = 'task_simple/search_task/'. trim($form_state['values']['filter']);
}

/* 
 * Process filter form submission when the Reset button is pressed.
 * (is not used here but good to know .)
 */
function task_simple_filter_form_submit_reset($form, &$form_state) {
  $form_state['redirect'] = 'task_simple/search_task/';
}

/* 
 * wrapper function for searcing for tasks. 
 * @param $keys (search terms)
 * @return html $output
 * 
 */
function task_simple_search_task($keys = NULL){
  $output = drupal_get_form('task_simple_filter_form', $keys);
  $sql = task_simple_create_list_query($keys, 'search');
  $output.= task_simple_generate_list($sql);
  return $output;
}
/* 
 * function for creating the list query
 * @param $keys (search terms)
 * @param String $op (type of list: default is future - other is past and all)
 * @return String $sql to be used to generate query
 */
function task_simple_create_list_query($keys = NULL, $op = 'future'){
  //we need user to know what tasks belongs to him  
  global $user;
  if (empty($keys) && $op == 'search'){
    return ''; // return empty query. No results will be shown as default on searching
  }
  
  $today = date('Y-m-d');
  //$progress = task_simple_get_progress();
  if ($keys) {
    //sql for a search
    $sql = "SELECT date(due) as due, datediff(due, '$today') as diff, tid, title, description, progress FROM {task} WHERE uid = '$user->uid' AND title LIKE '%".db_escape_string(trim($keys))."%'";
  } else {
    //sql for all tasks
    $sql = "SELECT date(due) as due, datediff(due, '$today') as diff, tid, title, description, progress FROM {task} WHERE uid = '$user->uid'";
  }

  if ($op == 'all'){
    return $sql;
  }

  if ($op == 'past'){
    //sql for past tasks (or finished)
    $sql.= " AND progress =5";
  }
  
  if ($op == 'future'){
    //sql for all future and unfinished tasks
    $sql.= " AND progress != 5";
  }
  return $sql; 
  
}

/* 
 * @param String $sql, $sql for generating list
 * @return String $output (html for displaying)
 */
function task_simple_generate_list($sql = NULL){

  $progress = task_simple_get_progress();
  //create headers of table views
  $header = 
    array(
      array('data' => t('Date'), 'field' => 'due', 'sort' => 'asc'),
      array('data' => t('Title'), 'field' => 'Title'),
      array('data' => t('Progress'), 'field' => 'Progress'),
      array('data' => t('Operations'), 'colspan' => '2'),

  );
  
  if (empty($sql)){
    return '';
  }

  //sort sql statement
  $sql .= tablesort_sql($header);
  //make paging query
  $result = pager_query($sql, TASK_SIMPLE_PAGER, 0, NULL);
  
  $rows = array();
  $destination = drupal_get_destination();

  //insert data for views
  while ($data = db_fetch_object($result)) {
    
    //rewrite diff row for showing tomorrow, today, yesterday
    if (task_simple_rewrite_diff($data->diff)){
      $date = task_simple_rewrite_diff($data->diff);
    } else {
      $date = $data->due;
    }
    
    $row = array(
      $date,
      check_plain($data->title),  
      $progress[$data->progress],
      l(t('edit'), "task_simple/edit_task/$data->tid", array('query' => $destination)), 
      l(t('delete'), "task_simple/delete_task/$data->tid", array('query' => $destination)),
    );
    $rows[] = $row;
  }

  if (empty($rows)) {
    $empty_message = t('No tasks found');
    $rows[] = array(array('data' => $empty_message, 'colspan' => 4));
  }

  //table theme
  $output .= theme('table', $header, $rows);
  $output .= theme('pager', NULL, 0, 0);
  return $output;
}

/* 
 * @param $due, the number of days to when the task is due
 * @returns string with info about days
 */
function task_simple_rewrite_diff($diff){
  if ($diff == 0){
    return t('Today');
  } elseif ($diff == 1){
    return t('Tomorrow');
  } elseif ($diff == -1){
    return t('Yesterday');
  } else {
    return 0;
  }
}

/* 
 * list all tasks in paging view and with different orders
 * @param $keys
 * @Returns a listing of all Tasks
 */
function task_simple_list_all() {
  $sql = task_simple_create_list_query($keys, 'all');
  $output = task_simple_generate_list($sql);
  return $output;
}

/* 
 * list all tasks in paging view and with different orders
 * @param $keys
 * @Returns a listing of all Tasks
 */
function task_simple_list_future($keys = NULL) {
  $sql = task_simple_create_list_query($keys, 'future');
  $output = task_simple_generate_list($sql);
  return $output;
}

/* 
 * list all tasks in paging view and with different orders
 * @param $keys
 * @returns a listing of all Tasks
 */
function task_simple_list_past($keys = NULL) {
  $sql = task_simple_create_list_query($keys, 'past');
  $output = task_simple_generate_list($sql);
  return $output;
}

/* 
 * Fetch a specific task from the database.
 * used for edit_task for setting the edit edit_task functions loaded values 
 * @param int the tid of the task
 * @return array with the task
 */

function task_simple_load($tid) {
  $query = 'SELECT tid, date(due) as due, title, description, progress FROM {task} WHERE tid = %d';
  return db_fetch_array(db_query($query, $tid));
}

/** 
 * wrapper function for adding a task 
 * @return string $html
 */
function task_simple_add_task() {
  $output =drupal_get_form('task_simple_user_form');
  return $output;
}

/**
 * wrapper function for editing a task
 * @param int of tid to be edited
 * @return string $html
 */
function task_simple_edit_task($tid) {
  if (!task_simple_check_user($tid)){
    drupal_set_message(t('Illigal action'));
    return '';
  }
  
  $task = task_simple_load($tid);
  drupal_set_title(check_plain($task['title']));
  $output = drupal_get_form('task_simple_user_form', $task);
  return $output;
}

/** 
 * function for checking if logged in user owns the task to prevent maluse
 * @param int $tid to be checked
 * @return int num_rows
 */
function task_simple_check_user($tid){
  global $user;
  $query = "SELECT count(tid) as num_rows FROM {task} WHERE uid = %d AND tid = %d";
  $result = db_query($query, $user->uid, $tid);
  $data = db_fetch_object($result);
  return $data->num_rows;
}

/**
 * @param $form
 * @param $form_state 
 * function for making some action when user_form has been submitted with success adds or edits a task
 */
function task_simple_user_form_submit($form, &$form_state) {
  //make user global, so we can se which user is logged in
  //we use the $user->uid for getting user id. 
  global $user;
  //update if hidden fields tid is in form
  if (isset($form_state['values']['tid'])){
    //update
    if (!task_simple_check_user($form_state['values']['tid'])){
      drupal_set_message(t('You can not edit tasks which does not belong to you!'));
      return;
    }
    //update
    $query = "UPDATE {task} SET title = '%s', description = '%s', progress= %d,  due ='%s' WHERE tid = %d";
    $result = db_query(
      $query, 
      $form_state['values']['title'], 
      $form_state['values']['description'], 
      $form_state['values']['progress'], 
      $form_state['values']['due'], 
      $form_state['values']['tid']
    );
    //set a message
    drupal_set_message(t('Task Edited'));
  } else {
    //insert
    $query = "INSERT INTO {task} (title, description, progress, uid, due) VALUES ('%s', '%s', %d, %d, '%s')";
    $result = db_query(
      $query, 
      $form_state['values']['title'], 
      $form_state['values']['description'], 
      $form_state['values']['progress'], 
      $user->uid, 
      $form_state['values']['due']
    );
    //set a message
    drupal_set_message(t('Task Inserted'));
  }
}

/**
 * function for showing a form for inserting or editing a task
 * @param &$form_state
 * @param $edit 
 * @returns user form for adding or editing tasks
 */
function task_simple_user_form(&$form_state, $edit = '') {

  //if there is no task to edit then we 
  //create a datestamp for inserting in database
  //Otherwise the default value is NULL, then
  //Drupal will generate todays date as default value 
  //for our due date
  
  if (!empty($edit)){

    $due = $edit['due'];
    //Also check to see if logged in user owns the task being edited
    if (!task_simple_check_user($edit['tid'])){
      drupal_set_message(t('You can not edit tasks which does not belong to you!'));
      return;
    }
    //Editing: start form
    $form['basic'] = array('#type' => 'fieldset',
      '#title' => t('Edit Task')
    );
  
  } else {
    $due = date('Y-m-d');
    //Adding: start form
    $form['basic'] = array('#type' => 'fieldset',
      '#title' => t('Add Task')
    );
  }
  
  $form['basic']['inline']['due'] = array(
    '#title' => t('Due date: Year-month-day'),
    '#type' => 'date_popup',
    '#date_format' => 'Y-m-d',
    '#required' => TRUE,
    '#default_value' => $due,
    '#size' => 10,
  );  

  $form['basic']['inline']['title'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#title' => t('Title'),
    '#default_value' => $edit['title'],
    '#maxlength' => 128
  );
  
  $form['basic']['inline']['description'] = array(
    '#type' => 'textarea',
    '#required' => FALSE,
    '#default_value' => $edit['description'],
    '#title' => t('Description'),
    '#maxlength' => 1024
  );
  
  $progress = task_simple_get_progress();
  $form['basic']['inline']['progress'] = array(
    '#type' => 'select',
    '#title' => t('Select Progress'),
    '#required' => TRUE,
    '#default_value' => $edit['progress'],
    '#options' => $progress,
  );

  if ($edit['tid']) {
    $form['tid'] = array('#type' => 'hidden', '#value' => $edit['tid']);
    $form['basic']['inline']['submit'] = array('#type' => 'submit', '#value' => t('Update Task'));
  }
  else {
    $form['basic']['inline']['submit'] = array('#type' => 'submit', '#value' => t('Add task'));
  }
  return $form;
}

/** 
 * @param $form
 * @param &$form_state
 * function for validating when user is editing or inserting tasks (only checking date)
 */
function task_simple_user_form_validate($form, &$form_state) {
  $ary = explode('-', $form_state['values']['due']);
  if (!checkdate($ary[1], $ary[2], $ary[0]) ) {
    form_set_error('due', t('You must enter a valid date'));
  }
}

/** 
 * @param $form_state
 * @param int (the task to delete)
 * Menu callback; confirms deleting a task
 * @return form for deleting a task
 */
function task_simple_delete_task($form_state, $tid) {
  $task = task_simple_load($tid);

  //check to se if user owns task
  if (!task_simple_check_user($tid)) {
    drupal_set_message(t('You can not delete tasks that does not belong to you!.'));
    return;
  }
  $form['tid'] = array('#type' => 'value', '#value' => $tid);
  $output = confirm_form(
    $form,
    t('Are you sure you want to delete task %title?', 
    array('%title' => $task['title'])),
    isset($_GET['destination']) ? $_GET['destination'] : 'task_simple');
  return $output;
}

/** 

 * @param $form
 * @param &$form_state
 * function for showing a confirm form when deleting a task
 */
function task_simple_delete_task_submit($form, &$form_state) {
  //check to se if user owns task
  if (!task_simple_check_user($form_state['values']['tid'])) {
    drupal_set_message(t('You can not delete tasks that does not belong to you!.'));
    return;
  }
  if ($form_state['values']['confirm']) {
    task_simple_delete($form_state['values']['tid']);
    $form_state['redirect'] = 'task_simple';
    return;
  }
}

/** 
 * @param int $tid
 * Post-confirmation: delete the task
 * function for delting the task
 */
function task_simple_delete($tid = 0) {
  db_query('DELETE FROM {task} WHERE tid = %d', $tid);
  drupal_set_message(t('The task has been deleted.'));
}
