;********************************************************
;*							*
;*							*
;*							*
;*							*
;*			University of Mexico		*
;*			Julio Cesar Cruz Estrda		*
;*							*
;*			17/06/2016			*
;*							*
;********************************************************
;
; Scene:
;	1) The robot waits for the instruction
;	


(deffacts scheduled_cubes

	(name-scheduled challenge 1 2)

	;	STATE 1	
	;
	(state (name challenge) (number 1)(duration 6000)(status active))
	(condition (conditional if) (arguments world status saw)(true-state 2)(false-state 1)(name-scheduled challenge)(state-number 1))
	(cd-task (cd cmdWhatSee) (actor robot)(obj robot)(from sensor)(to world)(name-scheduled challenge)(state-number 1))


	;	STATE 2
	; Robot wait for presentations
	(state (name challenge) (number 2)(duration 6000))
	(condition (conditional if) (arguments presentation status accomplished)(true-state 3)(false-state 2)(name-scheduled challenge)(state-number 2))
	(cd-task (cd cmdPresentation) (actor robot)(obj robot)(from sensor)(to presentation)(name-scheduled challenge)(state-number 2))

	;	STATE 3
	;The robot wait for questions about the enviroment and describe it
	(state (name challenge) (number 3)(duration 6000))
	(condition (conditional if) (arguments enviroment status described)(true-state 4)(false-state 3)(name-scheduled challenge)(state-number 3))
	(cd-task (cd cmdEnviroment) (actor robot)(obj robot)(from sensor)(to enviroment)(name-scheduled challenge)(state-number 3))



	;	STATE 4
	; The robot wait for a user instruction "the instruction will modify the enviroment"
	(state (name challenge) (number 4)(duration 6000)(status active))
	(condition (conditional if) (arguments enviroment status described)(true-state 4)(false-state 3)(name-scheduled challenge)(state-number 3))
	(cd-task (cd cmdEnviroment) (actor robot)(obj robot)(from sensor)(to enviroment)(name-scheduled challenge)(state-number 3))	
	
	
	
	
)

;;;;;;;;;;;;;;;;;;;;;; wait for initial command "what do you see"

(defrule task-what-you-see
	(state (name challenge) (number 1)(duration 6000)(status active))	
	(cd-task (cd cmdWhatSee) (actor robot)(obj robot)(from sensor)(to ?world)(name-scheduled challenge)(state-number 1))        
	=>
	(assert (plan (name what_you_see) (number 1)(actions question_world ?world)(duration 6000)))
	(assert (finish-planner what_you_see 1))
)

(defrule exe-plan-what-you-see
	(plan (name ?name) (number ?num-pln)(status active)(actions question_world ?world)(duration ?t))
 	;?f1 <- (item (name ?obj))
        =>
        (bind ?command (str-cat "" ?world ""))
        (assert (send-blackboard ACT-PLN cmd_world ?command ?t 4))
	;(waitsec 1)
        ;(assert (wait plan ?name ?num-pln ?t))
)

(defrule exe-plan-what-you-saw
        ?f <-  (received ?sender command cmd_world what_see_no 0)
        ?f2 <- (plan (name ?name) (number ?num-pln)(status active)(actions question_world ?world))
	?f1 <-(item (name ?world))
        =>
        (retract ?f)
        (modify ?f2 (status accomplished))
	(modify ?f1 (status saw))	
)

(defrule exe-plan-no-what-you-saw
        ?f <-  (received ?sender command cmd_world what_see_yes 1)
        ?f1 <- (item (name ?world))
        ?f2 <- (plan (name ?name) (number ?num-pln)(status active)(actions question_world ?world))
        =>
        (retract ?f)
        (modify ?f2 (status active))
)


;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;; Presentations


(defrule exe-plan-presentation
	(state (name challenge) (number 2)(duration 6000)(status active))	
	(cd-task (cd cmdPresentation) (actor robot)(obj robot)(from sensor)(to ?presentation)(name-scheduled challenge)(state-number 2))        
	=>
	(assert (plan (name presentations) (number 1)(actions first_source ?presentation)(duration 6000)))
	(assert (plan (name presentations) (number 2)(actions all_people ?presentation)(duration 6000)))
	(assert (finish-planner presentations 2))
)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;enviroment

(defrule exe-plan-enviroment
	(state (name challenge) (number 3)(duration 6000)(status active))	
	(cd-task (cd cmdEnviroment) (actor robot)(obj robot)(from sensor)(to ?envy)(name-scheduled challenge)(state-number 3))        
	=>
	(assert (plan (name presentations) (number 1)(actions first_source ?envy)(duration 6000)))
	(assert (plan (name presentations) (number 2)(actions all_people ?envy)(duration 6000)))
	(assert (finish-planner presentations 2))
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Instruction to modify enviroment





