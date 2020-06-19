# qloption
quantlib中需个性化的内容


1.新增雪球\凤凰奇异期权的定价策略
  采用蒙特卡洛计算
  使用有限差分方法  --todo
  
2.新增中国的日历类
  QuantLib已经增加了中国节假日的支持，但每年都需要重新编译整个工程。
  可以单独抽取出calender类，使用外部链接到自己的工程中。
  china.cpp 中节假日参考 http://www.sse.com.cn/disclosure/dealinstruc/closed/
