// #include <string>
// #include <vector>
// using namespace std;

// int dim = 0;
// vector<int>len;
// vector<int>Size;
// int cnt=0;
// struct e
// {
//     string type;
//     void* data;
//     vector<e*> kids;
//     int level;
// };
// e*root;
// void dfs(e* root)
// {
//     for(auto kid : root->kids)
//         dfs(kid);
//     int max_level = -1;
//     for(auto kid : root->kids)
//         max_level = max(max_level, kid->level);
//     for(int i=0;i<max_level;i++)
//         combine(root->kids, i);
// }
// void combine(vector<e*>&all, int level)
// {
//     vector<e*> new_all;
//     vector<e*> temp;
//     for(int i=0;i<all.size();i++)
//     {
//         if(all[i]->level == level)
//         {
//             temp.push_back(all[i]);
//             if(temp.size() == len[level])
//             {
//                 auto it = new e;
//                 temp.swap(it->kids);
//                 new_all.push_back(it);
//             }
//         }
//         else {
//             if(temp.size() > 0)
//             {
//                 auto it = new e;
//                 temp.swap(it->kids);
//                 new_all.push_back(it);
//             }
//             new_all.push_back(all[i]);
//         }
//     }
// }
// void init(e*initVal, int* ptr,int level)
// {
//     if(level == 0)
//     {
//         *ptr = *(int*)initVal->data;
//         return;
//     }
//     int step = Size[level-1];//TODO
//     for(int i=0;i<initVal->kids.size();i++)
//     {
//         init(initVal->kids[i],ptr + i*step, level-1);
//     }
// }