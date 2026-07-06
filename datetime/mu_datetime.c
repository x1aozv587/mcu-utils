#include "mu_datetime.h"

static bool mu_datetime_is_leap_year( uint16_t year );
static uint8_t mu_datetime_days_in_month( uint16_t year, uint8_t month );
static int64_t mu_datetime_days_from_2000( const mu_date_t *p_date );
static void mu_datetime_date_from_days( int64_t total_days, mu_date_t *p_date );
static uint32_t mu_datetime_secs_from_midnight( const mu_time_t *p_time );
static void mu_datetime_time_from_secs( uint32_t secs, mu_time_t *p_time );
static int64_t mu_datetime_days_min( void );
static int64_t mu_datetime_days_max( void );

static const uint8_t mu_datetime_month_days[12] =
{
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31,
};

/**< 判断闰年 */
static bool mu_datetime_is_leap_year( uint16_t year )
{
    if( year % 400U == 0U )
    {
        return true;
    }

    if( year % 100U == 0U )
    {
        return false;
    }

    if( year % 4U == 0U )
    {
        return true;
    }

    return false;
}

/**< 获取某月天数 */
static uint8_t mu_datetime_days_in_month( uint16_t year, uint8_t month )
{
    if( month < 1U || month > 12U )
    {
        return 0;
    }

    if( month == 2U && mu_datetime_is_leap_year( year ) == true )
    {
        return 29;
    }

    return mu_datetime_month_days[month - 1U];
}

/**< 合法年份下限（根据 MU_DATETIME_EPOCH） */
static uint16_t mu_datetime_year_min( void )
{
#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    return 1970U;
#else
    return 2000U;
#endif
}

/**< 合法年份上限 */
static uint16_t mu_datetime_year_max( void )
{
    return 2099U;
}

/**
 * @brief 从 2000-01-01 起的天数（可为负值表示 2000 年之前的日期）
 *
 * 内部统一以 2000-01-01 为基准计算天数偏移。
 * year >= 2000 时正向累加，year < 2000 时先累减整年再补当月已过的天数。
 */
static int64_t mu_datetime_days_from_2000( const mu_date_t *p_date )
{
    int64_t days = 0;

    if( p_date->year >= 2000U )
    {
        uint16_t y = 0;
        uint8_t m = 0;

        for( y = 2000; y < p_date->year; y++ )
        {
            days += mu_datetime_is_leap_year( y ) ? 366 : 365;
        }

        for( m = 1; m < p_date->month; m++ )
        {
            days += mu_datetime_days_in_month( p_date->year, m );
        }

        days += ( int64_t )( p_date->day - 1U );
    }
    else
    {
        uint16_t y = 0;
        uint8_t m = 0;

        for( y = p_date->year; y < 2000U; y++ )
        {
            days -= mu_datetime_is_leap_year( y ) ? 366 : 365;
        }

        for( m = 1; m < p_date->month; m++ )
        {
            days += mu_datetime_days_in_month( p_date->year, m );
        }

        days += ( int64_t )( p_date->day - 1U );
    }

    return days;
}

/**
 * @brief 从天数反推日期（以 2000-01-01 为基准）
 *
 * total_days >= 0 时从 2000 年起正向推算。
 * total_days < 0 时从 1999-12-31 往回推算。
 */
static void mu_datetime_date_from_days( int64_t total_days, mu_date_t *p_date )
{
    uint16_t y = 0;
    uint8_t m = 0;

    if( total_days >= 0 )
    {
        y = 2000;

        while( true )
        {
            int64_t year_days = mu_datetime_is_leap_year( y ) ? 366 : 365;

            if( total_days < year_days )
            {
                break;
            }

            total_days -= year_days;
            y++;
        }

        for( m = 1; m <= 12U; m++ )
        {
            uint8_t dim = mu_datetime_days_in_month( y, m );

            if( total_days < ( int64_t )dim )
            {
                break;
            }

            total_days -= dim;
        }

        p_date->day = ( uint8_t )( total_days + 1U );
    }
    else
    {
        total_days = -total_days - 1;
        y = 1999;

        while( true )
        {
            int64_t year_days = mu_datetime_is_leap_year( y ) ? 366 : 365;

            if( total_days < year_days )
            {
                break;
            }

            total_days -= year_days;
            y--;
        }

        for( m = 12; m >= 1U; m-- )
        {
            uint8_t dim = mu_datetime_days_in_month( y, m );

            if( total_days < ( int64_t )dim )
            {
                break;
            }

            total_days -= dim;
        }

        p_date->day = mu_datetime_days_in_month( y, m ) - ( uint8_t )total_days;
    }

    p_date->year  = y;
    p_date->month = m;
}

/**< 合法日期范围最小天数（2000-01-01 为基准） */
static int64_t mu_datetime_days_min( void )
{
    mu_date_t d;

    d.year  = mu_datetime_year_min();
    d.month = 1;
    d.day   = 1;

    return mu_datetime_days_from_2000( &d );
}

/**< 合法日期范围最大天数（2000-01-01 为基准） */
static int64_t mu_datetime_days_max( void )
{
    mu_date_t d;

    d.year  = mu_datetime_year_max();
    d.month = 12;
    d.day   = 31;

    return mu_datetime_days_from_2000( &d );
}

/**< 从午夜起秒数 */
static uint32_t mu_datetime_secs_from_midnight( const mu_time_t *p_time )
{
    return ( uint32_t )p_time->hour * 3600U +
           ( uint32_t )p_time->minute * 60U +
           ( uint32_t )p_time->second;
}

/**< 从秒数反推时间 */
static void mu_datetime_time_from_secs( uint32_t secs, mu_time_t *p_time )
{
    p_time->hour   = ( uint8_t )( ( secs / 3600U ) % 24U );
    p_time->minute = ( uint8_t )( ( secs / 60U ) % 60U );
    p_time->second = ( uint8_t )( secs % 60U );
}

bool mu_date_is_valid( const mu_date_t *p_date )
{
    if( p_date == NULL )
    {
        return false;
    }

    if( p_date->month < 1U || p_date->month > 12U )
    {
        return false;
    }

    if( p_date->day < 1U || p_date->day > mu_datetime_days_in_month( p_date->year, p_date->month ) )
    {
        return false;
    }

    if( p_date->year < mu_datetime_year_min() || p_date->year > mu_datetime_year_max() )
    {
        return false;
    }

    return true;
}

bool mu_time_is_valid( const mu_time_t *p_time )
{
    if( p_time == NULL )
    {
        return false;
    }

    if( p_time->hour > 23U ||
        p_time->minute > 59U ||
        p_time->second > 59U )
    {
        return false;
    }

    return true;
}

bool mu_datetime_is_valid( const mu_datetime_t *p_dt )
{
    if( p_dt == NULL )
    {
        return false;
    }

    return mu_date_is_valid( &p_dt->date ) && mu_time_is_valid( &p_dt->time );
}

bool mu_date_is_leap_year( uint16_t year )
{
    return mu_datetime_is_leap_year( year );
}

uint8_t mu_date_days_in_month( uint16_t year, uint8_t month )
{
    return mu_datetime_days_in_month( year, month );
}

mu_weekday_t mu_date_weekday( const mu_date_t *p_date )
{
    int64_t days = 0;

    if( mu_date_is_valid( p_date ) == false )
    {
        return MU_WEEKDAY_SUNDAY;
    }

    days = mu_datetime_days_from_2000( p_date );
    days = ( days % 7 + 7 ) % 7;

    return ( mu_weekday_t )( ( ( uint32_t )days + 6U ) % 7U );
}

uint16_t mu_date_day_of_year( const mu_date_t *p_date )
{
    uint8_t m = 0;
    uint16_t doy = 0;

    if( mu_date_is_valid( p_date ) == false )
    {
        return 0;
    }

    for( m = 1; m < p_date->month; m++ )
    {
        doy += mu_datetime_days_in_month( p_date->year, m );
    }

    doy += p_date->day;

    return doy;
}

uint32_t mu_datetime_to_timestamp( const mu_datetime_t *p_dt )
{
    uint32_t ts = 0;

    if( mu_datetime_to_timestamp_ex( p_dt, &ts ) == false )
    {
        return 0;
    }

    return ts;
}

bool mu_datetime_to_timestamp_ex( const mu_datetime_t *p_dt, uint32_t *p_timestamp )
{
    int64_t days = 0;
    int64_t secs = 0;

    if( mu_datetime_is_valid( p_dt ) == false || p_timestamp == NULL )
    {
        return false;
    }

    days = mu_datetime_days_from_2000( &p_dt->date );
    secs = days * 86400 + ( int64_t )mu_datetime_secs_from_midnight( &p_dt->time );

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    secs += ( int64_t )MU_SECONDS_1970_TO_2000;
#endif

    if( secs < 0 || secs > ( int64_t )0xFFFFFFFFUL )
    {
        return false;
    }

    *p_timestamp = ( uint32_t )secs;

    return true;
}

bool mu_timestamp_to_datetime( uint32_t timestamp, mu_datetime_t *p_dt )
{
    int64_t ts = 0;
    int64_t days = 0;
    uint32_t secs = 0;

    if( p_dt == NULL )
    {
        return false;
    }

    ts = ( int64_t )timestamp;

#if MU_DATETIME_EPOCH == MU_DATETIME_EPOCH_1970
    ts -= ( int64_t )MU_SECONDS_1970_TO_2000;
#endif

    if( ts >= 0 )
    {
        days = ts / 86400;
        secs = ( uint32_t )( ts % 86400 );
    }
    else
    {
        days = ( ts - 86399 ) / 86400;
        secs = ( uint32_t )( ts - days * 86400 );
    }

    mu_datetime_date_from_days( days, &p_dt->date );
    mu_datetime_time_from_secs( secs, &p_dt->time );

    return mu_datetime_is_valid( p_dt );
}

bool mu_date_add_days( mu_date_t *p_date, int32_t days )
{
    int64_t d = 0;

    if( mu_date_is_valid( p_date ) == false )
    {
        return false;
    }

    d = mu_datetime_days_from_2000( p_date ) + days;

    if( d < mu_datetime_days_min() || d > mu_datetime_days_max() )
    {
        return false;
    }

    mu_datetime_date_from_days( d, p_date );

    return true;
}

int32_t mu_date_diff_days( const mu_date_t *p_date1, const mu_date_t *p_date2 )
{
    int64_t d1 = 0;
    int64_t d2 = 0;
    int64_t diff = 0;

    if( mu_date_is_valid( p_date1 ) == false ||
        mu_date_is_valid( p_date2 ) == false )
    {
        return 0;
    }

    d1 = mu_datetime_days_from_2000( p_date1 );
    d2 = mu_datetime_days_from_2000( p_date2 );
    diff = d1 - d2;

    if( diff < ( int64_t )INT32_MIN || diff > ( int64_t )INT32_MAX )
    {
        return 0;
    }

    return ( int32_t )diff;
}

bool mu_time_add_seconds( mu_time_t *p_time, int32_t seconds )
{
    int32_t total = 0;

    if( mu_time_is_valid( p_time ) == false )
    {
        return false;
    }

    total = ( int32_t )mu_datetime_secs_from_midnight( p_time ) + seconds;

    if( total < 0 || total >= 86400 )
    {
        return false;
    }

    mu_datetime_time_from_secs( ( uint32_t )total, p_time );

    return true;
}

int8_t mu_date_compare( const mu_date_t *p_date1, const mu_date_t *p_date2 )
{
    if( p_date1 == NULL || p_date2 == NULL )
    {
        return 0;
    }

    if( p_date1->year > p_date2->year )
    {
        return 1;
    }

    if( p_date1->year < p_date2->year )
    {
        return -1;
    }

    if( p_date1->month > p_date2->month )
    {
        return 1;
    }

    if( p_date1->month < p_date2->month )
    {
        return -1;
    }

    if( p_date1->day > p_date2->day )
    {
        return 1;
    }

    if( p_date1->day < p_date2->day )
    {
        return -1;
    }

    return 0;
}

int8_t mu_time_compare( const mu_time_t *p_time1, const mu_time_t *p_time2 )
{
    if( p_time1 == NULL || p_time2 == NULL )
    {
        return 0;
    }

    if( p_time1->hour > p_time2->hour )
    {
        return 1;
    }

    if( p_time1->hour < p_time2->hour )
    {
        return -1;
    }

    if( p_time1->minute > p_time2->minute )
    {
        return 1;
    }

    if( p_time1->minute < p_time2->minute )
    {
        return -1;
    }

    if( p_time1->second > p_time2->second )
    {
        return 1;
    }

    if( p_time1->second < p_time2->second )
    {
        return -1;
    }

    return 0;
}
