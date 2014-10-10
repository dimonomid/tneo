
define \n


endef

TN_CALL_FROM_TASK     = \image html attr_call_task.png ${\n}\
                        \latexonly \
                        \includegraphics{../../images/attr_call_task.png} \
                        \endlatexonly \

TN_CALL_FROM_ISR      = \image html attr_call_int.png ${\n} \
                        \latexonly \
                        \includegraphics{../../images/attr_call_int.png} \
                        \endlatexonly \

TN_CALL_FROM_MAIN     = #{call from main()}

TN_CAN_SWITCH_CONTEXT = \image html attr_call_ct_sw.png ${\n} \
                        \latexonly \
                        \includegraphics{../../images/attr_call_ct_sw.png} \
                        \endlatexonly \


TN_CAN_SLEEP          = \image html attr_timeout.png ${\n} \
                        \latexonly \
                        \includegraphics{../../images/attr_timeout.png}    \
                        \endlatexonly \

TN_LEGEND_LINK        = <SMALL><I>(refer to \ref legend for details)</I></SMALL>

TN_SYS_TIMER_LINK     = <I>\ref time_ticks "system timer"</I>


TN_TASK_STATE_RUNNABLE  = \link TN_TASK_STATE_RUNNABLE <code>RUNNABLE</code>\endlink
TN_TASK_STATE_WAIT      = \link TN_TASK_STATE_WAIT <code>WAIT</code>\endlink
TN_TASK_STATE_SUSPEND   = \link TN_TASK_STATE_SUSPEND <code>SUSPEND</code>\endlink
TN_TASK_STATE_WAITSUSP  = \link TN_TASK_STATE_WAITSUSP <code>WAIT+SUSPEND</code>\endlink
TN_TASK_STATE_DORMANT   = \link TN_TASK_STATE_DORMANT <code>DORMANT</code>\endlink

TN_VERSION              = `bash ./hg_ver_echo.sh`


export TN_CALL_FROM_TASK
export TN_CALL_FROM_ISR
export TN_CALL_FROM_MAIN
export TN_CAN_SWITCH_CONTEXT
export TN_CAN_SLEEP

export TN_LEGEND_LINK

export TN_SYS_TIMER_LINK

export TN_TASK_STATE_RUNNABLE
export TN_TASK_STATE_WAIT
export TN_TASK_STATE_SUSPEND
export TN_TASK_STATE_WAITSUSP
export TN_TASK_STATE_DORMANT

all: 
	( cat tn_doxyfile ; echo "PROJECT_NUMBER=\"$(TN_VERSION)\"" ) | doxygen -

