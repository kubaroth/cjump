import sublime, sublime_plugin
import os
import subprocess
import re

## Todo move this stuff into the Project Settings
SYSTEM_INCLUDES = "/usr/include/c++/4.9/ /usr/include/x86_64-linux-gnu/c++/4.9/ /home/kuba/SRC/llvm/build/lib/clang/5.0.1/include" 
## initial version of system includes (gcc) for the project
#SYSTEM_INCLUDES = cpp -v < /dev/null 2>&1 | grep -oE "^[ ]*/[^ ;]*$" | tr '\n' ' '

CLANG_INCLUDES = "TODO"
CJUMP = "path TODO"
DEBUG = 0

class Cjump:
    def __init__(self):
        self.__project_data = sublime.active_window().project_data()    
        self.__includes = []
        self.__cjump_path = '/home/kuba/PRJ/cpp_rozne/clang_jump/build/cjump'
        self.tok = ''
        self.symbols = {}
        ### TODO on init
        '''
        - detect location of cjump, from the project data, if not configured ask for the valid path
        - update the project data
        '''

    @property
    def includes(self):
        return self.__includes
        
    @includes.setter
    def includes(self, paths):
        assert isinstance(paths, list)
        self.__includes = paths
    
    def collectIncludeDirs(self, includeDirsOnly=False):
        ## TODO: Check if the cached list sublimie dirs has change, if not reuse
        sublime_dirs = sublime.active_window().folders()
        self.__includes = []
        for top_level in sublime_dirs:
            if os.path.basename(top_level) == 'include': continue
            for root, dirs, files in os.walk(top_level):
                path = root.split(os.sep)
                if os.path.basename(root) == 'include':
                    self.__includes.append(root)
                    ## Exit on the first found include (TODO: could be added as a option)
                    break
                    
        if not includeDirsOnly: 
            self.__includes = self.__includes + sublime_dirs
        return self.__includes
                
    def __extractRegexName(self, str):
        arr = str.strip().split('|token_name: ')
        if len(arr) == 2:
            return arr[1]
        else: return None
    
    def __buildSymbolPath(self, strList):
        for token in strList:
            # print (token)
            name = re.findall('(?s)(?<=name: ).*?(?= type)', token)
            path = re.findall('(?s)(?<=path: ).*', token)
            # print (name, path)
            if len(path) > 0 and len(path) > 0:
                # print ("---", name[0], path[0])
                self.symbols[name[0]]= path[0]
        
    def run_cjump(self, mode='-def', source='', line=0, col=0):
        includes = ' '.join(self.__includes)
        includes = '. {0} {1}'.format(SYSTEM_INCLUDES, includes)
        
        #includes = '. /usr/include/c++/4.9/ /usr/include/x86_64-linux-gnu/c++/4.9/ /home/kuba/SRC/llvm/build/lib/clang/5.0.1/include /home/kuba/PRJ/cpp_rozne/StarSOP/vector_test/ /opt/hfs16.5/toolkit/include/'
        #source = '/opt/hfs16.5/toolkit/samples/SOP/SOP_Flatten.C'
        cmd = '{0} -def -source {1}:{2}:{3} {4}'.format(self.__cjump_path, source, line, col, includes)
        print (cmd)
        
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        cmd_stdout, cmd_stderr = p.communicate()
        stdout = cmd_stdout.decode("utf-8")
        print ("(error:)", cmd_stderr.decode("utf-8"))
        
        if DEBUG>=1: 
            for v in str(cmd_stdout).split('\\n'):  print ("(debug): ", v)
        
        tokenList = re.findall("^\| name.*",stdout, re.MULTILINE)
        nameList =  re.findall("^\|token_name.*",stdout, re.MULTILINE)

        if len(nameList) > 0:
            name = self.__extractRegexName(nameList[0])
            if not name:
                print ("-- No token under the caret")
            else:
                print (name)
                self.tok = name
        
        self.__buildSymbolPath(tokenList)
        if self.tok in self.symbols:
            path = self.symbols[self.tok]
            print (self.tok, path)
            return path
        else:
            print ("--", self.tok, "in cjump.symbols", "not found")
            #print (self.symbols.keys())
            return None
        
class CjumpCommand(sublime_plugin.TextCommand):
    
    def rowcol(self):
        '''match values with sublime status bar info'''
        point = self.view.sel()[0].begin()
        row,col = self.view.rowcol(point)
        row+=1
        col+=1
        return row,col

    def visitMember(self, compound_path):
        path, line, col = compound_path.split(":")
        w = sublime.active_window()
        point = self.view.text_point(int(line), int(col))
        region = sublime.Region(point,point)
        # vTemp = w.open_file('%s' % (compound_path), sublime.ENCODED_POSITION | sublime.TRANSIENT) ## Less tabs
        vTemp = w.open_file('%s' % (compound_path), sublime.ENCODED_POSITION)  ## DEBUG
        ## TODO: best will be to show in another View
        # vTemp.show(region)
        # vTemp.run_command("goto_line", {"line": line})
        
    def run(self, edit):
        cjmp = Cjump() 
        all_includes = cjmp.collectIncludeDirs()
        # print (all_includes)
        row, col = self.rowcol()
        compound_path = cjmp.run_cjump(mode='-def', source=self.view.file_name(), line=row, col=col)
        if compound_path:
            self.visitMember(compound_path)
