# Check if the plugin binary exists in the plugin dir
--let $plugin_dir= `SELECT @@GLOBAL.PLUGIN_DIR`
--file_exists $plugin_dir/mysql_privacy_plugin.so
let $privacy_plugin_early_installed= `SELECT COUNT(*) = 1
FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME = 'privacy_plugin'`;
let $sql_privacy_log_early_installed= `SELECT COUNT(*) = 1
FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME = 'SQL_PRIVACY_LOG'`;

# Install plugin
disable_query_log;
if (!$privacy_plugin_early_installed)
{
  INSTALL PLUGIN privacy_plugin SONAME 'mysql_privacy_plugin.so';
}
if (!$sql_privacy_log_early_installed)
{
  INSTALL PLUGIN SQL_PRIVACY_LOG SONAME 'mysql_privacy_plugin.so';
}
enable_query_log;

# Enable plugin
set @original_privacy_plugin_on=@@privacy_plugin_on;
set global privacy_plugin_on = 1;

# Increase PES Timeout
disable_query_log;
let $value = query_get_value(SHOW VARIABLES LIKE 'privacy_plugin_pes_overall_timeout', Value, 1);
if ($value !=  No such row)
{
  set @original_privacy_plugin_pes_overall_timeout=@@privacy_plugin_pes_overall_timeout;
  set global privacy_plugin_pes_overall_timeout = 60000;
}
enable_query_log;
