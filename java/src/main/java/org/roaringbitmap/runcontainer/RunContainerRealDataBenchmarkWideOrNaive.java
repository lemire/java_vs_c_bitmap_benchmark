package org.roaringbitmap.runcontainer;



import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Iterator;
import java.util.PriorityQueue;
import java.util.concurrent.TimeUnit;

import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.BenchmarkMode;
import org.openjdk.jmh.annotations.Mode;
import org.openjdk.jmh.annotations.OutputTimeUnit;
import org.openjdk.jmh.annotations.Param;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.Setup;
import org.openjdk.jmh.annotations.State;
import org.roaringbitmap.FastAggregation;
import org.roaringbitmap.RoaringBitmap;
import org.roaringbitmap.ZipRealDataRetriever;

import com.googlecode.javaewah32.EWAHCompressedBitmap32;

@BenchmarkMode(Mode.AverageTime)
@OutputTimeUnit(TimeUnit.MICROSECONDS)
@SuppressWarnings("rawtypes")
public class RunContainerRealDataBenchmarkWideOrNaive {


    // only include the first count items
    // Note: if you application is routinely aggregating
    // hundreds or thousands of bitmaps, you are maybe missing
    // optimization opportunities (e.g., one can precompute
    // some aggregates) so we mostly care for "moderate"
    // queries.
    protected static Iterator limit(final int count, final Iterator x) {

        return new Iterator(){
            int pos = 0;

            @Override
            public boolean hasNext() {
                return (pos < count) && (x.hasNext());
            }

            @Override
            public Object next() {
                pos++;
                return x.next();
            }

        };
    }



    @Benchmark
    public int RoaringWithRun(BenchmarkState benchmarkState) {
        int answer = RoaringBitmap.or(limit(benchmarkState.count,benchmarkState.rc.iterator()))
               .getCardinality();
        if(answer != benchmarkState.horizontalor)
            throw new RuntimeException("bug");
        return answer;
    }


    @Benchmark
    public int EWAH32_naive(BenchmarkState benchmarkState) {
        Iterator i = limit(benchmarkState.count,benchmarkState.ewah32.iterator());
        EWAHCompressedBitmap32 bitmapor = (EWAHCompressedBitmap32) i.next();
        while(i.hasNext())
            bitmapor = bitmapor.or((EWAHCompressedBitmap32) i.next());
        int answer = bitmapor.cardinality();
        if(answer != benchmarkState.horizontalor)
            throw new RuntimeException("bug");
        return answer;

    }



    @State(Scope.Benchmark)
    public static class BenchmarkState {
        @Param ({// putting the data sets in alpha. order
            "census-income", "census1881",
            "weather_sept_85", "wikileaks-noquotes",
            "census-income_srt","census1881_srt",
            "weather_sept_85_srt","wikileaks-noquotes_srt"
        })
        String dataset;

        int horizontalor = 0;

        protected int count = 8;// arbitrary number but warning: when increasing this number
        // check that reported timings increase monotonically, I found that as of ~12, they sharply decreased
        // for some schemes, suggesting that the benchmark was defeated.


        ArrayList<RoaringBitmap> rc = new ArrayList<RoaringBitmap>();
        ArrayList<RoaringBitmap> ac = new ArrayList<RoaringBitmap>();
        ArrayList<EWAHCompressedBitmap32> ewah32 = new ArrayList<EWAHCompressedBitmap32>();



        public BenchmarkState() {
        }

        @Setup
        public void setup() throws Exception {
            ZipRealDataRetriever dataRetriever = new ZipRealDataRetriever(dataset);
            System.out.println();
            System.out.println("Loading files from " + dataRetriever.getName());

            int normalsize = 0;
            int runsize = 0;
            int concisesize = 0;
            int wahsize = 0;
            int ewahsize = 0;
            int ewahsize32 = 0;
            long stupidarraysize = 0;
            long stupidbitmapsize = 0;
            int totalcount = 0;
            int numberofbitmaps = 0;
            int universesize = 0;
            for (int[] data : dataRetriever.fetchBitPositions()) {
                numberofbitmaps++;
                if(universesize < data[data.length - 1 ])
                    universesize = data[data.length - 1 ];
                stupidarraysize += 8 + data.length * 4L;
                stupidbitmapsize += 8 + (data[data.length - 1] + 63L) / 64 * 8;
                totalcount += data.length;
                EWAHCompressedBitmap32 ewahBitmap32 = EWAHCompressedBitmap32.bitmapOf(data);
                ewahsize32 += ewahBitmap32.serializedSizeInBytes();
                ewah32.add(ewahBitmap32);

                RoaringBitmap basic = RoaringBitmap.bitmapOf(data);
                RoaringBitmap opti = basic.clone();
                opti.runOptimize();
                rc.add(opti);
                ac.add(basic);
                normalsize += basic.serializedSizeInBytes();
                runsize += opti.serializedSizeInBytes();
            }
            count = rc.size();
            System.out.println("# aggregating the first "+count+" bitmaps out of "+ac.size());

            /***
             * This is a hack. JMH does not allow us to report
             * anything directly ourselves, so we do it forcefully.
             */
            DecimalFormat df = new DecimalFormat("0.0");
            System.out.println();
            System.out.println("==============");
            System.out.println("= data set "+dataset);
            System.out.println("Number of bitmaps = " + numberofbitmaps
                               + " total count = " + totalcount
                               + " universe size = "+universesize);
            System.out.println("Average bits per bitmap = "
                               + df.format(totalcount * 1.0 / numberofbitmaps));
            System.out.println("Run-roaring total     = "
                    + String.format("%1$10s", "" + runsize)
                    + "B, average per bitmap = "
                    + String.format("%1$10s",df.format(runsize * 1.0 / numberofbitmaps))
                    + "B, average bits per entry =  "
                    + String.format("%1$10s",df.format(runsize * 8.0 / totalcount)));
            System.out.println("EWAH 32-bit total = "
                    + String.format("%1$10s", "" + ewahsize32)
                    + "B, average per bitmap = "
                    + String.format("%1$10s",df.format(ewahsize32 * 1.0 / numberofbitmaps))
                    + "B, average bits per entry =  "
                    + String.format("%1$10s",df.format(ewahsize32 * 8.0 / totalcount)));
            System.out.println("Naive array total     = "
                    + String.format("%1$10s", "" + stupidarraysize)
                    + "B, average per bitmap = "
                    + String.format("%1$10s",df.format(stupidarraysize * 1.0 / numberofbitmaps))
                    + "B, average bits per entry =  "
                    + String.format("%1$10s",df.format(stupidarraysize * 8.0 / totalcount)));
            System.out.println("Naive bitmap total    = "
                    + String.format("%1$10s", "" + stupidbitmapsize)
                    + "B, average per bitmap = "
                    + String.format("%1$10s",df.format(stupidbitmapsize * 1.0 / numberofbitmaps))
                    + "B, average bits per entry =  "
                    + String.format("%1$10s",df.format(stupidbitmapsize * 8.0 / totalcount)));
            System.out.println("==============");
            System.out.println();
            horizontalor = FastAggregation.naive_or(limit(count,rc.iterator()))
                    .getCardinality();
        }

    }

}
