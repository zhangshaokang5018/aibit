#ifndef __NTP_H
#define __NTP_H

/**
 * @struct x_ntp_timestamp_t
 * @brief  NTP 时间戳。
 */
typedef struct x_ntp_timestamp_t
{
    unsigned int  xut_seconds;    ///< 从 1900年至今所经过的秒数
    unsigned int  xut_fraction;   ///< 小数部份，单位是微秒数的4294.967296( = 2^32 / 10^6 )倍
} x_ntp_timestamp_t;

/**
 * @enum  em_ntp_mode_t
 * @brief NTP工作模式的相关枚举值。
 */
typedef enum em_ntp_mode_t
{
    ntp_mode_unknow     = 0,  ///< 未定义
    ntp_mode_initiative = 1,  ///< 主动对等体模式
    ntp_mode_passive    = 2,  ///< 被动对等体模式
    ntp_mode_client     = 3,  ///< 客户端模式
    ntp_mode_server     = 4,  ///< 服务器模式
    ntp_mode_broadcast  = 5,  ///< 广播模式或组播模式
    ntp_mode_control    = 6,  ///< 报文为 NTP 控制报文
    ntp_mode_reserved   = 7,  ///< 预留给内部使用
} em_ntp_mode_t;

/**
 * @struct x_ntp_packet_t
 * @brief  NTP 报文格式。
 */
typedef struct x_ntp_packet_t
{
    unsigned char     xct_li_ver_mode;      ///< 2 bits，飞跃指示器；3 bits，版本号；3 bits，NTP工作    模式（参看 em_ntp_mode_t 相关枚举值）
    unsigned char     xct_stratum    ;      ///< 系统时钟的层数，取值范围为1~16，它定义了时钟的准确    度。层数为1的时钟准确度最高，准确度从1到16依次递减，层数为16的时钟处于未同步状态，不能作为参考时钟
    unsigned char     xct_poll       ;      ///< 轮询时间，即两个连续NTP报文之间的时间间隔
    unsigned char     xct_percision  ;      ///< 系统时钟的精度

    unsigned int      xut_root_delay     ;  ///< 本地到主参考时钟源的往返时间
    unsigned int      xut_root_dispersion;  ///< 系统时钟相对于主参考时钟的最大误差
    unsigned int      xut_ref_indentifier;  ///< 参考时钟源的标识

    x_ntp_timestamp_t xtmst_reference;      ///< 系统时钟最后一次被设定或更新的时间
    x_ntp_timestamp_t xtmst_originate;      ///< NTP请求报文离开发送端时发送端的本地时间
    x_ntp_timestamp_t xtmst_receive  ;      ///< NTP请求报文到达接收端时接收端的本地时间
    x_ntp_timestamp_t xtmst_transmit ;      ///< 应答报文离开应答者时应答者的本地时间
} x_ntp_packet_t;

extern inline unsigned long mktimes(unsigned int year, unsigned int mon, unsigned int day, unsigned int hour, unsigned int min, unsigned int sec)
{
    if (0 >= (int) (mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
        mon += 12;      /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return (((
                (unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
                year*365 - 719499
            )*24 + hour /* now have hours */
        )*60 + min /* now have minutes */
    )*60 + sec; /* finally seconds */
}
//mktimes(2019, 10, 11, 3, 01, 00);

#endif
