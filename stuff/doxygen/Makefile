
define \n


endef

# --- Images for bot html and latex documents
#
#     NOTE: to make it work, images should be copied to the latex output directory:
#     in your doxyfile, use the tag LATEX_EXTRA_FILES for that.

export TN_CALL_FROM_TASK
TN_CALL_FROM_TASK     = \image html attr_call_task.png ${\n}\
                        \latexonly \
                        \includegraphics{attr_call_task.png} \
                        \endlatexonly \

export TN_CALL_FROM_ISR
TN_CALL_FROM_ISR      = \image html attr_call_int.png ${\n} \
                        \latexonly \
                        \includegraphics{attr_call_int.png} \
                        \endlatexonly \

export TN_CALL_FROM_MAIN
TN_CALL_FROM_MAIN     = #{call from main()}

export TN_CAN_SWITCH_CONTEXT
TN_CAN_SWITCH_CONTEXT = \image html attr_call_ct_sw.png ${\n} \
                        \latexonly \
                        \includegraphics{attr_call_ct_sw.png} \
                        \endlatexonly \

export TN_CAN_SLEEP
TN_CAN_SLEEP          = \image html attr_timeout.png ${\n} \
                        \latexonly \
                        \includegraphics{attr_timeout.png}    \
                        \endlatexonly \



# --- Link to the legend

export TN_LEGEND_LINK
TN_LEGEND_LINK        = <SMALL><I>(refer to \ref legend for details)</I></SMALL>


# --- Link to the explanation of system timer

export TN_SYS_TIMER_LINK
TN_SYS_TIMER_LINK     = <I>\ref time_ticks "system timer"</I>


# --- Warning that symbol is available if only TN_DYNAMIC_TICK is set

export TN_IF_ONLY_DYNAMIC_TICK_SET
TN_IF_ONLY_DYNAMIC_TICK_SET      = <I>Available if only \link TN_DYNAMIC_TICK <code>TN_DYNAMIC_TICK</code> \endlink is <B>set</B>.</I>

export TN_IF_ONLY_DYNAMIC_TICK_NOT_SET
TN_IF_ONLY_DYNAMIC_TICK_NOT_SET  = <I>Available if only \link TN_DYNAMIC_TICK <code>TN_DYNAMIC_TICK</code> \endlink is <B>not set</B>.</I>


# --- Links to task states

export TN_TASK_STATE_RUNNABLE
export TN_TASK_STATE_WAIT
export TN_TASK_STATE_SUSPEND
export TN_TASK_STATE_WAITSUSP
export TN_TASK_STATE_DORMANT

TN_TASK_STATE_RUNNABLE  = \link TN_TASK_STATE_RUNNABLE <code>RUNNABLE</code>\endlink
TN_TASK_STATE_WAIT      = \link TN_TASK_STATE_WAIT <code>WAIT</code>\endlink
TN_TASK_STATE_SUSPEND   = \link TN_TASK_STATE_SUSPEND <code>SUSPEND</code>\endlink
TN_TASK_STATE_WAITSUSP  = \link TN_TASK_STATE_WAITSUSP <code>WAIT+SUSPEND</code>\endlink
TN_TASK_STATE_DORMANT   = \link TN_TASK_STATE_DORMANT <code>DORMANT</code>\endlink



# --- Kernel version
# (we don't export it since it is included in the make "all" target, see below)

TN_VERSION              = `bash ../scripts/git_ver_echo.sh`




all: 
	( cat tn_doxyfile ; echo "PROJECT_NUMBER=\"$(TN_VERSION)\"" ) | doxygen -

