package org.reigncarnated.menu;
import android.content.*;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.*;
/** Only two fixed, read-only files. No caller-controlled filesystem traversal. */
public final class DiagnosticsProvider extends ContentProvider {
 public boolean onCreate(){return true;}
 private File file(Uri uri)throws FileNotFoundException{
  String name=uri.getPath();if(!"/capture.png".equals(name)&&!"/report.txt".equals(name))throw new FileNotFoundException("Unknown diagnostic file");
  return new File(new File(getContext().getCacheDir(),"diagnostic-share"),name.substring(1));
 }
 public ParcelFileDescriptor openFile(Uri uri,String mode)throws FileNotFoundException{if(!"r".equals(mode))throw new FileNotFoundException("Read only");return ParcelFileDescriptor.open(file(uri),ParcelFileDescriptor.MODE_READ_ONLY);}
 public String getType(Uri uri){return "/capture.png".equals(uri.getPath())?"image/png":"text/plain";}
 public Cursor query(Uri uri,String[] projection,String selection,String[] args,String sort){
  try{File f=file(uri);String[] columns=projection==null?new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE}:projection;MatrixCursor c=new MatrixCursor(columns);Object[] row=new Object[columns.length];for(int i=0;i<row.length;i++)row[i]=OpenableColumns.DISPLAY_NAME.equals(columns[i])?f.getName():OpenableColumns.SIZE.equals(columns[i])?f.length():null;c.addRow(row);return c;}catch(IOException e){return null;}
 }
 public Uri insert(Uri u,ContentValues v){throw new UnsupportedOperationException("Read only");}
 public int delete(Uri u,String s,String[] a){throw new UnsupportedOperationException("Read only");}
 public int update(Uri u,ContentValues v,String s,String[] a){throw new UnsupportedOperationException("Read only");}
}
