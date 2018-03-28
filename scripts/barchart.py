# vim: encoding=utf-8

import math
import sys

import cairo
import pycha.bar


def generate_ticks(min_index, max_index):
  ticks = [
      dict(v=i, label=u'{0:,} \u2192 {1:,}'.format(2 ** i, 2 ** (i + 1) - 1)) \
          for i in xrange(min_index, max_index)
  ]

  if min_index == 0:
    ticks[0] = dict(v=0, label='0 -> 1')

  return ticks


def render(fout, label, val_type, val_max, vals):
  data = ((label, vals),)
  data_range = (vals[0][0], vals[-1][0])
  bar_count = data_range[1] - data_range[0] + 1

  interval = 10 ** int(math.ceil(math.log10(val_max)) - 1) / 2

  options = {
    'legend': {'hide': True},
    'title': label,
    'titleFont': 'Times New Roman',
    'titleFontSize': 32,
    'axis': {
      'labelFont': 'Times New Roman',
      'labelFontSize': 18,
      'tickFont': 'Times New Roman',
      'tickFontSize': 18,
      'x': {
          'ticks': generate_ticks(data_range[0], data_range[1] + 1),
          'label': val_type,
          'range': data_range,
        },
      'y': {'interval': interval, 'tickPrecision': 0, 'label': 'Count'},
    },
    'stroke': {'hide': True, 'width': 0, 'shadow': False},
    'barWidthFillFraction': 0.75,
    'colorScheme': {
        'name': 'gradient',
        'args': {'initialColor': '#de301b'},
    },
  }

  width = 1440
  height = 80 + 70 * bar_count

  surface = cairo.SVGSurface(fout, width, height)
  chart = pycha.bar.HorizontalBarChart(surface, options)
  chart.addDataset(data)
  chart.render()

  surface.flush()


if __name__ == '__main__':
  global hist_filename
  hist_filename = 'mysql_real_query'
  data = [
      [18, 3563], # 2^19 - 1 = 524287
      [19, 1615], # 2^20 - 1 = 1048575
      [20, 1],    # 2^21 - 1 = 2097151
    ]

  render(sys.stdout, hist_filename, 'nsecs', 3563, data)
