/*
 * Copyright (C) 2017 Tieto Poland Sp. z o. o.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <qcom_mem.h>

#include "json_float.h"
#include "base.h"

/*
 * Private auxiliary function
 */

static A_UINT32 calculate_multiplier(A_UINT8 precision)
{
  A_UINT32 ret = 1;
  A_UINT8 i;

  for (i = 0; i < precision; ++i)
  {
    ret *= 10;
  }

  return ret;
}

static A_INT32 string_to_fraction(A_CHAR *fract_str, A_UINT8 precision)
{
  A_UINT32 fract;
  A_CHAR *buff;

  if (precision == 0)
  {
    return 0;
  }

  /* Limit size of string */
  buff = qcom_mem_calloc(precision + 1, sizeof(char));

  strncpy(buff, fract_str, precision);

  buff[precision] = '\0';

  if (precision > strlen(buff))
  {
    memset(buff + strlen(buff), '0', precision - strlen(buff));
  }

  fract = atoi(buff);

  qcom_mem_free(buff);

  return fract;
}

static void precision_align_up(json_float_t *f1, json_float_t *f2)
{
  if (f1->precision < f2->precision)
  {
    A_UINT32 multiplier = calculate_multiplier(f2->precision - f1->precision);

    f1->value *= multiplier;
    f1->precision = f2->precision;
  }
  else if (f1->precision > f2->precision)
  {
    A_UINT32 multiplier = calculate_multiplier(f1->precision - f2->precision);

    f2->value *= multiplier;
    f2->precision = f1->precision;
  }
}

static void insert_char_back(A_CHAR *str, A_CHAR c, A_UINT32 pos)
{
  A_INT32 i;

  /* Remember about terminator character */
  ++pos;

  for (i = strlen(str) + 1; pos != -1; --i, --pos)
  {
    str[i + 1] = str[i];

    if (pos == 0)
    {
      str[i] = c;
    }
  }
}

/*
 * Public API function
 */

json_float_t json_float(json_float_sign_t sign, A_UINT32 int_part, A_UINT32 fract_part, A_UINT8 precision)
{
  A_UINT32 multiplier;
  A_INT32 value;

  multiplier = calculate_multiplier(precision);

  value = multiplier * int_part;

  /* Fit fractional part to requested precision */
  while (multiplier <= fract_part)
  {
    fract_part /= 10;
  }

  /* Join intager and fractional to one variable */
  value += fract_part;

  if (sign == JSON_FLOAT_SIGN_MINUS)
  {
    value *= -1;
  }

  return (json_float_t){value, precision};
}

A_UINT8 json_float_get_precision_from_string(const A_CHAR *str)
{
  A_CHAR *decimal_point;
  A_UINT8 i;

  if (str == NULL)
  {
    return 0;
  }

  decimal_point = strchr(str, '.');
  if (decimal_point == NULL)
  {
    return 0;
  }

  for(i = 0; decimal_point[i + 1] != '\0'; ++i);

  return i;
}

json_float_t json_float_from_string(const A_CHAR *str, A_UINT8 precision)
{
  A_UINT32 fract_part = 0;
  A_UINT32 int_part = 0;
  A_CHAR *decimal_point;
  json_float_sign_t sign = JSON_FLOAT_SIGN_PLUS;

  if (str[0] == '-')
  {
    sign = JSON_FLOAT_SIGN_MINUS;
  }

  int_part = strtoul(str + sign, NULL, 10);

  decimal_point = strchr(str, '.');
  if (decimal_point != NULL)
  {
    /* Get character after decimal point */
    fract_part = string_to_fraction(decimal_point + 1, precision);
  }

  return json_float(sign, int_part, fract_part, precision);
}

void json_float_to_string(const json_float_t *f, A_CHAR *str)
{
  json_float_sign_t sign = JSON_FLOAT_SIGN_PLUS;
  A_UINT8 dot_position = f->precision;

  sprintf(str, "%d", f->value);

  if (str[0] == '-')
  {
    sign = JSON_FLOAT_SIGN_MINUS;
  }

  /* Insert '0' after decimal point if fractional part is too small */
  while (strlen(str) - sign < dot_position)
  {
    insert_char_back(str, '0', strlen(str) - sign);
  }

  /* Insert dot between intager and fractional part. If precision is equal zero, fractional part does not exist */
  if (dot_position != 0)
  {
    insert_char_back(str, '.', dot_position);
  }

  /* Append '0' to begin of string if necessary */
  if (str[sign] == '.')
  {
    insert_char_back(str, '0', strlen(str) - sign);
  }
}

json_float_t json_float_add(json_float_t f1, json_float_t f2)
{
  precision_align_up(&f1, &f2);

  return (json_float_t){f1.value + f2.value, f1.precision};
}

json_float_t json_float_sub(json_float_t f1, json_float_t f2)
{
  precision_align_up(&f1, &f2);

  return (json_float_t){f1.value - f2.value, f1.precision};
}

json_float_t json_float_mul_const(json_float_t f, float constant)
{
  return (json_float_t){f.value * constant, f.precision};
}
