#include <cstdio>
#include <cstdlib>
#include <clang-c/Index.h>

#include "clang/Tooling/CommonOptionsParser.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

using std::cout;
using std::endl;

using namespace clang::tooling;
using namespace llvm;

// ./cjump -def -source /home/kuba/PRJ/cpp_rozne/test_vector.cpp:16:6 . /usr/include/c++/4.9/ /usr/include/x86_64-linux-gnu/c++/4.9/ /home/kuba/PRJ/cpp_rozne/StarSOP/vector_test/ /opt/hfs16.5/toolkit/include
// ./cjump  -def -source /opt/hfs16.5/toolkit/samples/SOP/SOP_Flatten.C:5:5 . /usr/include/c++/4.9/ /usr/include/x86_64-linux-gnu/c++/4.9/ /home/kuba/PRJ/cpp_rozne/StarSOP/vector_test/ /opt/hfs16.5/toolkit/include/
cl::opt<std::string> InputFile("source",cl::desc("<source_file>"),cl::Required);
cl::list<std::string> IncludePaths(cl::Positional ,cl::desc("<includes> [... <sourceN>]"),cl::OneOrMore);
cl::opt<bool> Definition("def",cl::desc("Go to current definition"),cl::ValueOptional);
cl::opt<bool> Declaration("decl",cl::desc("Go to current declaration"),cl::ValueOptional);
cl::opt<bool> Currenttype("type",cl::desc("Show the type of variable at curent location"),cl::ValueOptional);
cl::opt<bool> Complete ("complete",cl::desc("Complete at point"),cl::ValueOptional);



struct Local{
    
  static std::vector<std::string> test(std::string input){
        std::istringstream ss(input);        
        std::string token;
        std::vector<std::string> tokens;
        while(std::getline(ss, token, ':')) {
            tokens.push_back(token);
        }
        return tokens;
    }
};

char *convert(const std::string & s)
{
   char *pc = new char[s.size()+1];
   std::strcpy(pc, s.c_str());
   return pc; 
}


std::string find_token(const CXTranslationUnit &tu, const CXToken *tokens, unsigned numTokens, int myline, int mycol) {
    printf("=== show tokens ===\n");
    printf("NumTokens: %d\n", numTokens);
    
    std::string tok_name = "";
    
    for (auto i = 0U; i < numTokens; i++) {
        const CXToken &token = tokens[i];
        CXTokenKind kind = clang_getTokenKind(token);
        CXString spell = clang_getTokenSpelling(tu, token);
        CXSourceLocation loc = clang_getTokenLocation(tu, token);
        
        
        CXFile file;
        unsigned line, column, offset;
        clang_getFileLocation(loc, &file, &line, &column, &offset);
        
        
        // cout << line << " " << myline << " " << column << " " << mycol << "___" << (line == myline) << " " << (0==0) <<  endl;
        if (line < myline) {continue;}
        if ((line == myline) && (column <= mycol) ){            
            CXString fileName = clang_getFileName(file);
            clang_disposeString(fileName);
            tok_name = clang_getCString(spell);
        }            
        else {
            break; 
        }
        
        clang_disposeString(spell);
    }
    return tok_name;
}

//////////////////


const char *_getTokenKindSpelling(CXTokenKind kind) {
    switch (kind) {
    case CXToken_Punctuation: return "Punctuation"; break;
    case CXToken_Keyword:     return "Keyword"; break;
    case CXToken_Identifier:  return "Identifier"; break;
    case CXToken_Literal:     return "Literal"; break;
    case CXToken_Comment:     return "Comment"; break;
    default:                  return "Unknown"; break;
    }
}

void show_all_tokens(const CXTranslationUnit &tu, const CXToken *tokens, unsigned numTokens) {
    printf("=== show tokens ===\n");
    printf("NumTokens: %d\n", numTokens);
    for (auto i = 0U; i < numTokens; i++) {
        const CXToken &token = tokens[i];
        CXTokenKind kind = clang_getTokenKind(token);
        CXString spell = clang_getTokenSpelling(tu, token);
        CXSourceLocation loc = clang_getTokenLocation(tu, token);
        
        CXFile file;
        unsigned line, column, offset;
        clang_getFileLocation(loc, &file, &line, &column, &offset);
        CXString fileName = clang_getFileName(file);
        
        printf("Token: %d\n", i);
        printf(" Text: %s\n", clang_getCString(spell));
        printf(" Kind: %s\n", _getTokenKindSpelling(kind));
        printf(" Location: %s:%d:%d:%d\n",
               clang_getCString(fileName), line, column, offset);
        printf("\n");
        
        clang_disposeString(fileName);
        clang_disposeString(spell);
    }
}

unsigned get_filesize(const char *fileName) {
    FILE *fp = fopen(fileName, "r");
    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    fclose(fp);
    return size;
}

CXSourceRange get_filerange(const CXTranslationUnit &tu, const char *filename) {
    CXFile file = clang_getFile(tu, filename);
    auto fileSize = get_filesize(filename);
    
    // get top/last location of the file
    CXSourceLocation topLoc  = clang_getLocationForOffset(tu, file, 0);
    CXSourceLocation lastLoc = clang_getLocationForOffset(tu, file, fileSize);
    if (clang_equalLocations(topLoc,  clang_getNullLocation()) ||
            clang_equalLocations(lastLoc, clang_getNullLocation()) ) {
        printf("cannot retrieve location\n");
        exit(1);
    }
    
    // make a range from locations
    CXSourceRange range = clang_getRange(topLoc, lastLoc);
    if (clang_Range_isNull(range)) {
        printf("cannot retrieve range\n");
        exit(1);
    }
    
    return range;
}

void show_clang_version(void) {
    CXString version = clang_getClangVersion();
    printf("%s\n", clang_getCString(version));
    clang_disposeString(version);
}

std::string getCursorKindName( CXCursorKind cursorKind )
{
    CXString kindName = clang_getCursorKindSpelling( cursorKind );
    std::string result = clang_getCString( kindName );
    clang_disposeString( kindName );
    return result;
}
std::string getCursorSpelling( CXCursor cursor )
{
    CXString cursorSpelling = clang_getCursorSpelling( cursor );
    std::string result
            = clang_getCString( cursorSpelling );
    clang_disposeString( cursorSpelling );
    return result;
}

CXChildVisitResult visitor( CXCursor cursor, CXCursor /* parent */, CXClientData clientData )
{
    CXSourceLocation location = clang_getCursorLocation( cursor );
    if( clang_Location_isFromMainFile( location ) == 0 )
        return CXChildVisit_Continue;
    CXCursorKind cursorKind = clang_getCursorKind( cursor );
    unsigned int curLevel = *( reinterpret_cast<unsigned int*>( clientData ) );
    unsigned int nextLevel = curLevel + 1;
    
    CXCursor def = clang_getCursorDefinition(cursor);    
    if (!clang_Cursor_isNull(def)){
        auto spelling = clang_getCString( clang_getCursorSpelling( cursor ) );
            
        CXSourceRange c_range = clang_getCursorExtent( def );
        CXSourceLocation loc = clang_getCursorLocation(def);
        CXFile file;
        unsigned int line = 0, col = 0, offset = 0;
        clang_getSpellingLocation(loc, &file, &line, &col, &offset);
        
        // TODO ClangCString need to be disposed
        
        std::string thetype = clang_getCString(clang_getTypeSpelling(clang_getCursorType(cursor)));
        cout << spelling << " " << thetype << " " << clang_getCString(clang_getFileName(file))<<":" << line << ":" << col << endl;
    }
    
    clang_visitChildren( cursor, visitor, &nextLevel );
    
    return CXChildVisit_Continue;
}

int main(int argc, const char **argv) {
    
    cl::ParseCommandLineOptions(argc, argv);
                   
    cout << Definition << endl;
    cout << Declaration <<endl;
    cout << Complete <<endl;
    cout << Currenttype <<endl;
    cout << InputFile <<endl;
    for (auto & p : IncludePaths){
        cout << "___" << p <<endl;
    }
    
    // TODO: detect if at least one optional option is selected (-def should be default)
    
    
    show_clang_version();
    
    
//    const auto filename = argv[1];
//    const auto cmdArgs = &argv[2];
//    auto numArgs = argc - 2;
    
    // create index w/ excludeDeclsFromPCH = 1, displayDiagnostics=1.
    CXIndex index = clang_createIndex(1, 1);
    
    cout << IncludePaths.size() <<endl;
    
    // Includes have to also point to the stdlib directories, Clang does not include this by default
    // The number of arguments has to match the entries in the array.
//   const char *args[] = {
//       "-I.", 
//       "-I/home/kuba/PRJ/cpp_rozne/StarSOP/vector_test/",
//       "-I/opt/hfs16.5/toolkit/include",
//       "-I/usr/include/x86_64-linux-gnu/c++/4.9/", 
//       "-I/usr/include/c++/4.9"
//   };

     auto input_vector = Local::test(InputFile.c_str());
     if (input_vector.size() != 3){
         return 1;
     }
     const auto filename = input_vector[0].c_str();
     auto input_line = std::stoi(input_vector[1]);
     auto input_col =  std::stoi(input_vector[2]);
     
     std::cout << "line: " << input_line << " col: " << input_col << std::endl;
     
    // const auto filename = InputFile.c_str();
    
//    if (IncludePaths.size() > 0){
         
        // Option 1 - create a vector of char* - pass the address to the first element
        std::vector<std::string> vs{ std::begin(IncludePaths), std::end(IncludePaths) };
        for(std::string &path : vs){      
            path = "-I"+path;
            cout << path <<endl;
         }
         std::vector<const char*>  vc;
         std::transform(vs.begin(), vs.end(), std::back_inserter(vc), convert);  
         
//         /// Option 2 Create a new string (not array) this does not work with clang_parseTranslationUnit()
//        std::string new_string;
//        new_string = std::accumulate(IncludePaths.begin(), IncludePaths.end(), new_string,
//              [] (const std::string& s1, const std::string& s2) -> std::string { 
//                 return s1.empty() ? "-I" + s2 : s1 + " -I" + s2; } );         
//        cout << "+++" <<new_string<< endl;
//        cout << new_string.size() <<endl;
              
//    }
        
    CXTranslationUnit tu = clang_parseTranslationUnit(index, filename, &vc[0], IncludePaths.size(), NULL, 0, CXTranslationUnit_None);

    if (tu == NULL) {
        printf("Cannot parse translation unit\n");
        return 1;
    }
    
    
    // get CXSouceRange of the file
    CXSourceRange range = get_filerange(tu, filename);
    
    // tokenize in the range
    CXToken *tokens;
    unsigned numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);
    
    /// TODO: do we really need tokenize as we already  know the position 
    //  and the word we are trying to match. The only problem was not accurate extration of the name
    //  so tokenization may help here to find the closest match - (low priority)
    // show tokens
    // show_all_tokens(tu, tokens, numTokens);
    auto tok_name = find_token(tu, tokens, numTokens, input_line, input_col /*line, col*/);  
    cout << "tok_name: " <<tok_name <<endl;
    
    
    ///////////////////////
    //Text: rrr
    //Kind: Identifier
    //Location: /home/kuba/PRJ/cpp_rozne/test_vector.cpp:15:5:194
    
    CXCursor rootCursor = clang_getTranslationUnitCursor( tu );
    //  clang_getCursorExtent()
    //  clang_getCursorDefinition()        
    
    CXCursorKind cursorKind = clang_getCursorKind( rootCursor );
    auto result = clang_getCString( clang_getCursorKindSpelling( cursorKind ) );
    CXSourceRange c_range = clang_getCursorExtent( rootCursor );
    cout << result << " " << c_range.begin_int_data << " " << c_range.end_int_data << endl;
    CXCursor def = clang_getCursorDefinition(rootCursor);
    
    unsigned int treeLevel = 0;
    clang_visitChildren( rootCursor, visitor, &treeLevel );
    
    //////////
    
    clang_disposeTokens(tu, tokens, numTokens);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    
    return 0;
}
