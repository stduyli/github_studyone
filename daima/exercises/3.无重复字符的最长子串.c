/*
 * @lc app=leetcode.cn id=3 lang=c
 *
 * [3] 无重复字符的最长子串
 */

// @lc code=start
int lengthOfLongestSubstring(char* s) {
    int len = strlen(s);
    if (len == 0) return 0;
    
    int maxLen = 0;
    int start = 0;
    int charMap[128] = {0}; // 使用数组记录每个字符最后出现的位置
    
    for (int end = 0; end < len; end++) {
        // 如果当前字符已经出现过，更新start位置
        if (charMap[s[end]] > start) {
            start = charMap[s[end]];
        }
        // 更新当前字符的位置
        charMap[s[end]] = end + 1;
        // 更新最大长度
        maxLen = (end - start + 1) > maxLen ? (end - start + 1) : maxLen;
    }
    
    return maxLen;
}
// @lc code=end

