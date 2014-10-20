" config file for Vim's plugin "vimprj"

" path to .vimprj folder
let s:sPath = expand('<sfile>:p:h')

" set .indexer_file to use
let g:indexer_indexerListFilename = s:sPath.'/.indexer_files'

let g:indexer_getAllSubdirsFromIndexerListFile = 1

" project-specific format settings
let &tabstop = 3
let &shiftwidth = 3

let s:o_dir = $INDEXER_PROJECT_ROOT.'/output/tmp/compiled_in_vim'

if !isdirectory(s:o_dir)
   call mkdir(s:o_dir, "p")
endif

let g:indexer_handlePath = 0

call CheckNeededSymbols(
         \  "The following items needs to be defined in your vimfiles/machine_local_conf/current/variables.vim file to make things work: ",
         \  "",
         \  [
         \     '$MACHINE_LOCAL_CONF__PATH__MICROCHIP_XC32',
         \  ]
         \  )

let s:sCompilerExecutable = ''
if has('win32') || has('win64')
   let s:sCompilerExecutable = 'xc32-gcc.exe'
else
   let s:sCompilerExecutable = 'xc32-gcc'
endif

" Path to MPLAB .mcp project
let s:sProject = 'arch/pic32/example_queue_pic32.X'
call envcontrol#set_project_file($INDEXER_PROJECT_ROOT.'/'.s:sProject, 'MPLAB_X', {
         \        'parser_params': {
         \           'compiler_command_without_includes' : ''
         \              .'"'.s:sCompilerExecutable.'" '
         \              .' -g -D__DEBUG -x c -c -mprocessor=32MX440F512H -Wall -mips16'
         \              .' -Os -MMD -MF "'.s:o_dir.'/%:t:r.o.d" -o "'.s:o_dir.'/%:t:r.o"',
         \        },
         \        'handle_clang' : 1,
         \        'add_paths' : [
         \           $MACHINE_LOCAL_CONF__PATH__MICROCHIP_XC32.'/lib/gcc/pic32mx/4.5.2/include',
         \           $MACHINE_LOCAL_CONF__PATH__MICROCHIP_XC32.'/pic32mx/include',
         \        ],
         \        'clang_add_params' : ''
         \              .'  -D __C32__'
         \              .'  -D __C32_VERSION__=1210'
         \              .'  -D __CLANG_FOR_COMPLETION__'
         \              .'  -D __LANGUAGE_C__'
         \              .'  -D __PIC32MX__'
         \              .'  -D __32MX440F512H__'
         \              .'  -D __PIC32_FEATURE_SET__=440'
         \              .'  -D __DEBUG'
         \              .'  -Wno-builtin-requires-header'
         \              .'  -Wno-unused-value'
         \              .'  -Wno-implicit-function-declaration'
         \              .'  -Wno-attributes'
         \              .'  -Wno-invalid-source-encoding'
         \              ,
         \ })

