package org.roaringbitmap.realdata.wrapper;


import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.List;

import org.roaringbitmap.RoaringBitmap;

import com.googlecode.javaewah32.EWAHCompressedBitmap32;


public final class BitmapFactory {

  public static final String EWAH32 = "ewah32";
  public static final String ROARING_WITH_RUN = "roaring_with_run";
  public static final String ROARING_ONLY = "ROARING_ONLY";
  public static final String BITMAP_TYPES = "BITMAP_TYPES";

  private static final List<File> TEMP_FILES = new ArrayList<File>();

  private BitmapFactory() {}


  public static Bitmap newEwah32Bitmap(int[] data) {
    EWAHCompressedBitmap32 ewah32 = EWAHCompressedBitmap32.bitmapOf(data);
    return new Ewah32BitmapWrapper(ewah32);
  }

  public static Bitmap newRoaringWithRunBitmap(int[] data) {
    return newRoaringBitmap(data, true);
  }

  private static Bitmap newRoaringBitmap(int[] data, boolean optimize) {
    RoaringBitmap roaring = RoaringBitmap.bitmapOf(data);
    if (optimize) {
      roaring.runOptimize();
    }
    return new RoaringBitmapWrapper(roaring);
  }

}
