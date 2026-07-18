#include "Debug.h"


void EasyLogger_Init(void)
{
  // 初始化 elog
  elog_init();
  // 使能文字颜色
  elog_set_text_color_enabled(true);
  // 设置日志格式
  elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~(ELOG_FMT_P_INFO | ELOG_FMT_T_INFO | ELOG_FMT_TIME));
  elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL& ~(ELOG_FMT_P_INFO | ELOG_FMT_T_INFO | ELOG_FMT_TIME));
  elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_ALL & ~(ELOG_FMT_P_INFO | ELOG_FMT_T_INFO | ELOG_FMT_TIME));
  elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_ALL & ~(ELOG_FMT_P_INFO | ELOG_FMT_T_INFO | ELOG_FMT_TIME));
  elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_P_INFO | ELOG_FMT_T_INFO | ELOG_FMT_TIME));
  elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_P_INFO | ELOG_FMT_T_INFO | ELOG_FMT_TIME));
  // 开启日志
  elog_start();
}

