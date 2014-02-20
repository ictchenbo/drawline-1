/* 
 *   ICT Lexical Analysis Platform
 *   
 *       Liu Bingyang
 *       Liu Qian
 *       Zhong Yanqin
 *       Wu Dayong
 *
 *   File Creator: Liu Bingyang
 *   Create Time: 2013-05-09
 *
 */


#ifndef ICT_LA_PLATFORM_CORE_NE
#define ICT_LA_PLATFORM_CORE_NE

#include <list>

#ifdef WIN32
    #ifdef ICTLAP_CORE_NE_EXPORT
        #define ICTLAP_CORE_NE_DLL _declspec (dllexport)
    #else
        #define ICTLAP_CORE_NE_DLL _declspec (dllimport)
    #endif
#else
    #define ICTLAP_CORE_NE_DLL 
#endif

namespace ICTLAP {
    namespace CORE {
        
        /*Named Entity Recognition*/
        namespace NE {
            
            enum NEtype { peo, org, loc, locC, stock};
            
            struct annoteNode {
                long    offset;
                long    len;
                NEtype  tp;
            };

            // 命名实体结果结构体
            class NEresult {
                char* _origText;
                char* _toStr;
            
              public:
                std::list<annoteNode> labels;

                NEresult();
                NEresult(const char*);
                ~NEresult();

                void innerPrint();
                const char* toString(){
                    return _toStr;
                }
            };

            
            typedef void* NEapi;
            typedef NEresult* NEres;
            
            /*!
             *  \fn ICTLAP_CORE_NE_DLL NEapi Init(const char* confFileName)
             *  \brief 命名实体模块全局初始化
             *  \param [in] confFileName 配置文件全路径
             *  \return 成功返回非空指针，否则返回NULL
             *  \author liubingyang
             */  
            ICTLAP_CORE_NE_DLL NEapi Init(const char* confFileName);
            
            /*!
             * \fn ICTLAP_CORE_NE_DLL NEres Parse(NEapi h, const char* text, long len)
             * \brief 计算命名实体结果主函数
             * \param [in] h Init初始化返回的指针
             * \param [in] text 需要处理的文本
             * \param [in] len 文本的长度
             * \return 命名实体结果结构体指针
             * \author liubingyang
             */ 
            ICTLAP_CORE_NE_DLL NEres Parse(NEapi h, const char* text, long len);
            
            /*!
             * \fn ICTLAP_CORE_NE_DLL void  ReleaseRes(NEapi h, NEres)
             * \brief 释放命名实体结果空间
             * \param [in] h Init初始化返回的指针
             * \param [in] NEres Parse返回的结果指针
             * \return 无
             * \author liubingyang
             */ 
            ICTLAP_CORE_NE_DLL void  ReleaseRes(NEapi h, NEres);
            
            /*!
             * \fn ICTLAP_CORE_NE_DLL void  Exit(NEapi h)
             * \brief 命名实体模块全局退出
             * \param [in] h Init初始化返回的指针
             * \return 无
             * \author liubingyang
             */ 
            ICTLAP_CORE_NE_DLL void  Exit(NEapi h);
            
            /*!
             * \fn ICTLAP_CORE_NE_DLL void Version()
             * \brief 打印版本信息
             * \return 无
             * \author liubingyang
             */ 
            ICTLAP_CORE_NE_DLL void  Version();

        }
    }
}


/*********************************************************************************/
extern "C" {
    // this part is for cross language lib reference
    // use extern C can generate funtions with static names
    // Another reason is extern "C" is not compatitable with namespaces
    // only basic types of variables are used here
    
    /*
     * 此部分是为了跨语言的调用和链接
     * 使用 extern C 是为了在链接表中输出固定不变的名字
     * 此外 exterc "C" 和 namespace 不能兼容，所以本部分的函数的
     * 功能与上面的函数完全相同但是命名更长防止冲突
     */
    typedef void* CORE_NE_API;
    
    ICTLAP_CORE_NE_DLL CORE_NE_API CORE_NE_Init(const char* confFileName);
    ICTLAP_CORE_NE_DLL char* CORE_NE_Parse(CORE_NE_API h, const char* text, long len);
    ICTLAP_CORE_NE_DLL void CORE_NE_ReleaseRes(CORE_NE_API h, char* buf);
    ICTLAP_CORE_NE_DLL void CORE_NE_Exit(CORE_NE_API h);
    ICTLAP_CORE_NE_DLL void CORE_NE_Version();
}
/********************************************************************************/

#endif
