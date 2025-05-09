/*
// 
                        
//   _____                  __
//  /\   _`\               /\ \  
//  \ \ \L\ \  __  __    __\_\ \  
//   \ \  _ < /\ \/\ \  /\  __  \  
//    \ \ \L\ \ \ \_\ \ \ \ \L\  \  
//     \ \____/\ \____ \ \ \_____/  
//      \/___/  \/___/\ \ \/____/
//                   __\ \             
//                  /\ ___\           
//                  \/____/
//
*/

#define _REPORT_ERROR 1
#ifdef _REPORT_ERROR 
    #define _NOT_IMPLEMENTED(msg) if(_REPORT_ERROR) throw std::logic_error(std::string("This function is not implemented: ")+msg);


    #define _PRECONDITION(argument,errMsg) if(!(argument)){ if(_REPORT_ERROR){ throw std::logic_error(std::string("Precondition: ")+errMsg);}}
    #define _NONNULL(argument) if(!(argument))throw std::runtime_error(std::string("NullPointerException!"));;
    #define _NULL(argument) if((argument))throw std::runtime_error(std::string("NullPointerException!"));;
    #define _ERROR(msg) if(_REPORT_ERROR)throw std::runtime_error(std::string("Runtime Erro: ")+msg);
    #define _LOGIC(msg) if(_REPORT_ERROR)throw std::logic_error(std::string("Wrong Logic! ")+msg);
#else 
    #define _NOT_IMPLEMENTED(msg) 


    #define _PRECONDITION(argument,errMsg) 

    #define _ERROR(msg) 
    #define _LOGIC(msg) 
#endif
#define _PRINT_DEBUG 0
#ifdef  _PRINT_DEBUG
    #define _DEBUG(msg) if( _PRINT_DEBUG)std::cerr << "Debugg: " << (msg) << std::endl;
#else
    #define _DEBUG(msg) // 什么也不做
#endif
#define _PRINT_WALKTRACE 0
#ifdef  _PRINT_DEBUG
    #define _INTER(msg) if( _PRINT_DEBUG)std::cerr << "Walk: " << (msg) << std::endl;
#else
    #define _INTER(msg) // 什么也不做
#endif
