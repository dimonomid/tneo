
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

TN_LEGEND_LINK        = (refer to \ref legend for details)

export TN_CALL_FROM_TASK
export TN_CALL_FROM_ISR
export TN_CALL_FROM_MAIN
export TN_CAN_SWITCH_CONTEXT
export TN_CAN_SLEEP

export TN_LEGEND_LINK

all: 
	doxygen tn_doxyfile

