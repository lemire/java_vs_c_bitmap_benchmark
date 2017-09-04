package org.roaringbitmap;

import static org.roaringbitmap.realdata.wrapper.BitmapFactory.BITMAP_TYPES;
import static org.roaringbitmap.realdata.wrapper.BitmapFactory.EWAH32;
import static org.roaringbitmap.realdata.wrapper.BitmapFactory.ROARING_ONLY;
import static org.roaringbitmap.realdata.wrapper.BitmapFactory.ROARING_WITH_RUN;
import static org.roaringbitmap.realdata.wrapper.BitmapFactory.newEwah32Bitmap;
import static org.roaringbitmap.realdata.wrapper.BitmapFactory.newRoaringWithRunBitmap;

import java.util.ArrayList;
import java.util.List;

import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.State;
import org.openjdk.jmh.annotations.TearDown;
import org.roaringbitmap.realdata.wrapper.Bitmap;

@State(Scope.Benchmark)
public abstract class AbstractBenchmarkState {

  public List<Bitmap> bitmaps;

  public AbstractBenchmarkState() {}

  public void setup(String dataset, String type) throws Exception {

    if (ROARING_ONLY.equals(System.getProperty(BITMAP_TYPES))
        && !ROARING_WITH_RUN.equals(type)) {
      throw new RuntimeException(String.format("Skipping non Roaring type %s", type));
    }

    bitmaps = new ArrayList<Bitmap>();


    ZipRealDataRetriever dataRetriever = new ZipRealDataRetriever(dataset);


    for (int[] data : dataRetriever.fetchBitPositions()) {
      Bitmap bitmap = null;

      if (EWAH32.equals(type)) {
          bitmap = newEwah32Bitmap(data);
      } else if (ROARING_WITH_RUN.equals(type)) {
          bitmap = newRoaringWithRunBitmap(data);
      }

      if (bitmap == null) {
        throw new RuntimeException(
            String.format("Unsupported parameters: type=%s", type));
      }

      bitmaps.add(bitmap);
    }

  }


}
