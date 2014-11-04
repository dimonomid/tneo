
" detect path to .vimprj dir
let s:sPath = expand('<sfile>:p:h')

let g:proj_project_filename=s:sPath.'/.vimprojects'

let &tabstop = 3
let &shiftwidth = 3
set colorcolumn=81

let g:indexer_ctagsDontSpecifyFilesIfPossible = 1
"let g:indexer_ctagsJustAppendTagsAtFileSave = 1
let g:indexer_indexerListFilename = s:sPath.'/.indexer_files'
let g:indexer_handlePath = 0


let g:indexer_ctagsCommandLineOptions = '--c++-kinds=+p+l --c-kinds=+l --fields=+iaS --extra=+q'

let s:o_dir = $INDEXER_PROJECT_ROOT.'/arch/pic32/tneokernel_pic32.X/build/tmp/compiled_in_vim'

if !isdirectory(s:o_dir)
   call mkdir(s:o_dir, "p")
endif

let s:sCompilerExecutable = ''
if has('win32') || has('win64')
   let s:sCompilerExecutable = 'xc32-gcc.exe'
else
   let s:sCompilerExecutable = 'xc32-gcc'
endif

call CheckNeededSymbols(
         \  "The following items needs to be defined in your vimfiles/machine_local_conf/current/variables.vim file to make things work: ",
         \  "",
         \  [
         \     '$MACHINE_LOCAL_CONF__PATH__MICROCHIP_XC32',
         \  ]
         \  )

let s:sProject = 'arch/pic32/tneokernel_pic32.X'

call envcontrol#set_project_file($INDEXER_PROJECT_ROOT.'/'.s:sProject, 'MPLAB_X', {
         \        'parser_params': {
         \           'compiler_command_without_includes' : ''
         \              .'"'.s:sCompilerExecutable.'" '
         \              .' -g -D__DEBUG -x c -c -mprocessor=32MX440F512H -Wall '
         \              .' -Os -MMD -MF "'.s:o_dir.'/%:t:r.o.d" -o "'.s:o_dir.'/%:t:r.o"',
         \        },
         \        'handle_clang' : 1,
         \        'add_paths' : [
         \           $MACHINE_LOCAL_CONF__PATH__MICROCHIP_XC32.'/lib/gcc/pic32mx/4.5.2/include',
         \           $MACHINE_LOCAL_CONF__PATH__MICROCHIP_XC32.'/pic32mx/include',
         \        ],
         \        'clang_add_params' : ''
         \              .'  -D __C32__'
         \              .'  -D __CLANG_FOR_COMPLETION__'
         \              .'  -D __LANGUAGE_C__'
         \              .'  -D __PIC32MX__'
         \              .'  -D __32MX440F512H__'
         \              .'  -D __DEBUG'
         \              .'  -Wno-builtin-requires-header'
         \              .'  -Wno-unused-value'
         \              .'  -Wno-implicit-function-declaration'
         \              .'  -Wno-attributes'
         \              .'  -Wno-invalid-source-encoding'
         \              ,
         \ })


